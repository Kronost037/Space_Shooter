#include "game.h"
#include "menu.h"

#include <math.h>
#include <stdbool.h>

#ifndef TAU
#define TAU 6.28318530718f
#endif

static bool s_titleIntroPlayed = false;
static float s_titleGlowTimer = -1.0f;

static float menu_clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static float menu_smoothstep(float a, float b, float x) {
    float t = menu_clampf((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static Color menu_alpha(Color c, unsigned char a) {
    c.a = a;
    return c;
}

static void drawSoftGlow(Vector2 p, float radius, Color c) {
    for (int i = 5; i >= 1; --i) {
        float t = (float)i / 5.0f;
        unsigned char a = (unsigned char)((float)c.a * t * t * 0.55f);
        DrawCircleV(p, radius * t, menu_alpha(c, a));
    }
}

static void drawGlintSweep(Rectangle r, float timer, Color accent, float intensity) {
    float w = r.width * 0.26f;
    float x = r.x - w + fmodf(timer * 120.0f, r.width + w * 2.0f);

    DrawRectangleGradientH(
        (int)x,
        (int)(r.y + 2.0f),
        (int)w,
        (int)(r.height - 4.0f),
        menu_alpha((Color){ 255, 255, 255, 0 }, 0),
        menu_alpha(accent, (unsigned char)(34.0f * intensity))
    );
}

static void drawButton(Game *game, const Button *button, bool hovered, Color accent, float pulse, float press, float appear, bool primary, float glowBoost) {
    Rectangle r = button->bounds;

    appear = menu_clampf(appear, 0.0f, 1.0f);
    float introLift = (1.0f - appear) * 18.0f;
    float hoverLift = hovered ? (primary ? 4.0f : 3.0f) : 0.0f;
    float pressDrop = press * 2.0f;

    float hoverScale = hovered ? (1.0f + 0.018f * game->menu->hoverPulse) : 1.0f;
    float w = r.width * hoverScale;
    float h = r.height * hoverScale;

    Rectangle body = {
        r.x + (r.width - w) * 0.5f,
        r.y - hoverLift + pressDrop + introLift + (r.height - h) * 0.5f,
        w,
        h
    };

    unsigned char a = (unsigned char)(255.0f * appear);
    unsigned char shadowA = (unsigned char)(86.0f * appear);

    DrawRectangleRounded(
        (Rectangle){ body.x + 4.0f, body.y + 7.0f, body.width, body.height },
        0.22f, 12,
        (Color){ 0, 0, 0, shadowA }
    );

    Color fillTop = hovered ? (Color){ 30, 38, 64, (unsigned char)(235.0f * appear) } : (Color){ 18, 24, 42, (unsigned char)(220.0f * appear) };
    Color fillBottom = hovered ? (Color){ 10, 14, 28, (unsigned char)(225.0f * appear) } : (Color){ 8, 10, 20, (unsigned char)(212.0f * appear) };

    DrawRectangleRounded(body, 0.22f, 12, fillTop);
    DrawRectangleRounded(
        (Rectangle){ body.x, body.y + body.height * 0.44f, body.width, body.height * 0.56f },
        0.22f, 12, fillBottom
    );

    DrawRectangleRounded(
        (Rectangle){ body.x + 2.0f, body.y + 2.0f, body.width - 4.0f, body.height - 4.0f },
        0.22f, 12,
        (Color){ 255, 255, 255, (unsigned char)(16.0f * appear) }
    );

    DrawRectangleLinesEx(
        body,
        2.0f,
        hovered ? menu_alpha((Color){ 120, 220, 255, 255 }, (unsigned char)(190.0f * appear)) : menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)(54.0f * appear))
    );

    DrawRectangleRec(
        (Rectangle){ body.x + 11.0f, body.y + 10.0f, 5.0f, body.height - 20.0f },
        menu_alpha(accent, a)
    );

    DrawRectangleRec(
        (Rectangle){ body.x + 10.0f, body.y + 6.0f, body.width - 20.0f, 2.0f },
        menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)((hovered ? 34.0f : 18.0f) * appear))
    );

    drawGlintSweep(body, game->menu->timer + pulse, accent, appear);

    if (hovered) {
        drawSoftGlow(
            (Vector2){ body.x + body.width * 0.5f, body.y + body.height * 0.5f },
            body.width * 0.54f * glowBoost,
            menu_alpha(accent, (unsigned char)(24.0f * appear))
        );
    }

    float fontSize = primary ? (hovered ? 43.0f : 41.0f) : (hovered ? 24.0f : 22.0f);
    float fontSpacing = primary ? 2.0f : 1.5f;
    Vector2 textSize = MeasureTextEx(game->font, button->label, fontSize, fontSpacing);
    Vector2 textPos = {
        body.x + body.width / 2.0f - textSize.x / 2.0f,
        body.y + body.height / 2.0f - textSize.y / 2.0f - 2.0f
    };

    DrawTextEx(
        game->font,
        button->label,
        (Vector2){ textPos.x + 1.5f, textPos.y + 1.5f },
        fontSize, fontSpacing,
        menu_alpha((Color){ 0, 0, 0, 120 }, a)
    );

    DrawTextEx(
        game->font,
        button->label,
        textPos,
        fontSize, fontSpacing,
        hovered ? menu_alpha((Color){ 245, 248, 255, 255 }, a) : menu_alpha((Color){ 220, 225, 235, 255 }, a)
    );
}

static void startTransition(Menu *menu, MenuAction action) {
    menu->pendingAction = action;
    menu->phase = MENU_PHASE_TRANSITION;
    menu->transitionTimer = 0.0f;
    menu->clickFlash = 1.0f;
    PlaySound(menu->click_sfx);
}

void refreshLayout(Game *game) {
    game->gameWidth = SCREEN_WIDTH - (game->showPanel ? PANEL_WIDTH : 0);
    game->gameHeight = SCREEN_HEIGHT;
}

void updateMenuLayout(Game *game) {
    Rectangle menuPanel = {
        SCREEN_WIDTH * 0.5f - 330.0f,
        118.0f,
        660.0f,
        578.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    Rectangle content = {
        menuPanel.x + 52.0f,
        menuPanel.y + 210.0f,
        menuPanel.width - 104.0f,
        308.0f
    };

    game->menu->startButton.bounds = (Rectangle){
        content.x + 12.0f,
        content.y + 10.0f,
        content.width - 24.0f,
        58.0f
    };

    float halfGap = 12.0f;
    float smallW = (content.width - 24.0f - halfGap) * 0.5f;

    game->menu->leaderboardButton.bounds = (Rectangle){
        content.x + 12.0f,
        content.y + 86.0f,
        smallW,
        52.0f
    };

    game->menu->settingButton.bounds = (Rectangle){
        content.x + 12.0f + smallW + halfGap,
        content.y + 86.0f,
        smallW,
        52.0f
    };

    game->menu->exitButton.bounds = (Rectangle){
        content.x + 12.0f,
        content.y + 160.0f,
        content.width - 24.0f,
        58.0f
    };

    (void)centerX;
}

void initializeMenu(Menu *menu) {
    *menu = (Menu){0};

    menu->bg_song = LoadMusicStream("src/Assets/menu.mp3");
    menu->hover_sfx = LoadSound("src/Assets/menu_button_hover.mp3");
    menu->click_sfx = LoadSound("src/Assets/menu_button_click.mp3");

    menu->mousePos = (Vector2){ 0.0f, 0.0f };
    menu->timer = 0.0f;
    menu->titlePulse = s_titleIntroPlayed ? 1.0f : 0.0f;

    menu->transitionTimer = 0.0f;
    menu->transitionDuration = 1.28f;
    menu->phase = MENU_PHASE_IDLE;
    menu->pendingAction = MENU_ACTION_NONE;

    menu->hoveredButton = -1;
    menu->hoverPulse = 0.0f;
    menu->clickFlash = 0.0f;

    menu->startButton.label = "START";
    menu->leaderboardButton.label = "LEADERBOARD";
    menu->settingButton.label = "SETTINGS";
    menu->exitButton.label = "EXIT";
}

bool menuActionReady(const Menu *menu) {
    return menu->phase == MENU_PHASE_TRANSITION &&
           menu->transitionTimer >= menu->transitionDuration &&
           menu->pendingAction != MENU_ACTION_NONE;
}

MenuAction menuConsumeAction(Menu *menu) {
    if (!menuActionReady(menu)) return MENU_ACTION_NONE;

    MenuAction action = menu->pendingAction;
    menu->pendingAction = MENU_ACTION_NONE;
    menu->phase = MENU_PHASE_IDLE;
    menu->transitionTimer = 0.0f;
    menu->clickFlash = 0.0f;
    return action;
}

static float menu_easeOutBack(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float t = x - 1.0f;
    return 1.0f + c3 * t * t * t + c1 * t * t;
}


static void drawTitleGlowLetters(Game *game, const char *text, Vector2 pos, float fontSize, float spacing, float appear, float glowT) {
    Font font = game->font;
    float totalWidth = MeasureTextEx(font, text, fontSize, spacing).x;

    float cursorX = pos.x;
    float sweepCenter = pos.x + totalWidth * (0.08f + 0.84f * glowT);
    float band = totalWidth * 0.18f;

    for (const char *p = text; *p; ++p) {
        char ch[2] = { *p, '\0' };
        Vector2 cs = MeasureTextEx(font, ch, fontSize, spacing);
        float charW = cs.x;

        float charCenter = cursorX + charW * 0.5f;
        float dist = fabsf(charCenter - sweepCenter);

        float glow = 1.0f - menu_clampf(dist / band, 0.0f, 1.0f);
        glow = glow * glow * (3.0f - 2.0f * glow);

        float localBeat = 0.5f + 0.5f * sinf(glowT * TAU * 1.4f + charCenter * 0.02f);
        float hot = glow * appear * (0.65f + 0.35f * localBeat);

        Color cyan = (Color){ 70, 40, 235, (unsigned char)(180.0f * hot) };
        Color violet = (Color){ 40, 50, 52, (unsigned char)(130.0f * hot) };
        Color white = (Color){ 235, 248, 255, (unsigned char)(255.0f * appear) };

        DrawTextEx(font, ch, (Vector2){ cursorX - 2.0f, pos.y }, fontSize, spacing, menu_alpha(cyan, cyan.a));
        DrawTextEx(font, ch, (Vector2){ cursorX + 2.0f, pos.y }, fontSize, spacing, menu_alpha(violet, violet.a));
        DrawTextEx(font, ch, (Vector2){ cursorX, pos.y - 1.0f }, fontSize, spacing, menu_alpha(cyan, (unsigned char)(120.0f * hot)));
        DrawTextEx(font, ch, (Vector2){ cursorX, pos.y + 1.0f }, fontSize, spacing, menu_alpha(violet, (unsigned char)(100.0f * hot)));

        DrawTextEx(font, ch, (Vector2){ cursorX, pos.y }, fontSize, spacing, white);

        cursorX += charW + spacing;
    }
}

void runMenuPhysics(Game *game) {
    Menu *menu = game->menu;

    UpdateMusicStream(menu->bg_song);

    float dt = GetFrameTime();
    menu->timer += dt;

    if (menu->titlePulse < 1.0f) {
        menu->titlePulse += dt * 0.42f;
        if (menu->titlePulse >= 1.0f) {
            menu->titlePulse = 1.0f;
            s_titleIntroPlayed = true;
            if (s_titleGlowTimer < 0.0f) s_titleGlowTimer = 0.0f;
        }
    } else if (s_titleGlowTimer >= 0.0f && s_titleGlowTimer < 1.15f) {
        s_titleGlowTimer += dt * 0.85f;
    }

    if (menu->timer > 1000.0f) menu->timer = 0.0f;

    if (menu->hoverPulse > 0.0f) {
        menu->hoverPulse -= dt * 2.2f;
        if (menu->hoverPulse < 0.0f) menu->hoverPulse = 0.0f;
    }

    if (menu->clickFlash > 0.0f) {
        menu->clickFlash -= dt * 2.8f;
        if (menu->clickFlash < 0.0f) menu->clickFlash = 0.0f;
    }

    menu->mousePos = GetMousePosition();

    updateMenuLayout(game);

    bool startHover = CheckCollisionPointRec(menu->mousePos, menu->startButton.bounds);
    bool leaderHover = CheckCollisionPointRec(menu->mousePos, menu->leaderboardButton.bounds);
    bool settingHover = CheckCollisionPointRec(menu->mousePos, menu->settingButton.bounds);
    bool exitHover = CheckCollisionPointRec(menu->mousePos, menu->exitButton.bounds);

    int hovered = startHover ? 0 : leaderHover ? 1 : settingHover ? 2 : exitHover ? 3 : -1;

    if (hovered != menu->hoveredButton) {
        if (hovered != -1) PlaySound(menu->hover_sfx);
        menu->hoveredButton = hovered;
        menu->hoverPulse = 1.0f;
    }

    if (menu->phase == MENU_PHASE_IDLE) {
        if (startHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_START);
        } else if (exitHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_EXIT);
        }
    } else if (menu->phase == MENU_PHASE_TRANSITION) {
        menu->transitionTimer += dt;
    }
}

static void drawMenuDecor(const Menu *menu, Rectangle panel) {
    float t = menu->timer;
    float cx = panel.x + panel.width * 0.5f;
    float titleY = panel.y + 72.0f;

    drawSoftGlow((Vector2){ cx, titleY + 8.0f }, 130.0f, (Color){ 80, 170, 255, 24 });
    drawSoftGlow((Vector2){ cx - 140.0f, titleY + 2.0f }, 86.0f, (Color){ 140, 70, 220, 18 });
    drawSoftGlow((Vector2){ cx + 148.0f, titleY + 10.0f }, 78.0f, (Color){ 60, 200, 255, 14 });

    DrawRing((Vector2){ cx, titleY + 6.0f }, 40.0f, 68.0f, 22.0f, 340.0f, 64, (Color){ 60, 200, 255, 22 });
    DrawRing((Vector2){ cx, titleY + 6.0f }, 24.0f, 35.0f, 10.0f, 350.0f, 48, (Color){ 145, 30, 230, 14 });

    for (int i = 0; i < 5; ++i) {
        float a = t * 0.7f + (float)i * 1.55f;
        Vector2 p = {
            cx + cosf(a) * (96.0f + i * 11.0f),
            titleY + sinf(a * 1.13f) * (22.0f + i * 2.0f)
        };

        Color c = (i % 2 == 0) ? (Color){ 60, 200, 255, 255 } : (Color){ 145, 30, 230, 255 };
        DrawCircleV(p, 2.0f + (float)(i % 3), menu_alpha(c, 86));
    }

    DrawRectangleRec(
        (Rectangle){ panel.x + 22.0f, panel.y + 18.0f, panel.width - 44.0f, 2.0f },
        (Color){ 60, 200, 255, 124 }
    );
    DrawRectangleRec(
        (Rectangle){ panel.x + 22.0f, panel.y + 22.0f, panel.width - 124.0f, 1.0f },
        (Color){ 145, 30, 230, 86 }
    );
}

void drawMenu(Game *game) {
    Menu *menu = game->menu;

    drawBackground(SCREEN_WIDTH, SCREEN_HEIGHT, menu->timer);

    Rectangle menuPanel = {
        SCREEN_WIDTH * 0.5f - 330.0f,
        118.0f,
        660.0f,
        578.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    DrawRectangleRec(
        (Rectangle){ 0, menuPanel.y + 20.0f, SCREEN_WIDTH, 2.0f },
        (Color){ 145, 30, 230, 26 }
    );
    DrawRectangleRec(
        (Rectangle){ 0, menuPanel.y + 23.0f, SCREEN_WIDTH, 1.0f },
        (Color){ 60, 200, 255, 16 }
    );

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x - 8.0f, menuPanel.y - 8.0f, menuPanel.width + 16.0f, menuPanel.height + 16.0f },
        0.08f, 16,
        (Color){ 145, 30, 230, 18 }
    );

    DrawRectangleRounded(menuPanel, 0.08f, 16, (Color){ 12, 16, 30, 220 });

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 2.0f, menuPanel.width - 4.0f, 176.0f },
        0.08f, 16,
        (Color){ 255, 255, 255, 10 }
    );

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 190.0f, menuPanel.width - 4.0f, menuPanel.height - 192.0f },
        0.08f, 16,
        (Color){ 0, 0, 0, 30 }
    );

    DrawRectangleLinesEx(menuPanel, 2.0f, (Color){ 255, 255, 255, 44 });
    DrawRectangleLinesEx(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 2.0f, menuPanel.width - 4.0f, menuPanel.height - 4.0f },
        1.0f, (Color){ 145, 30, 230, 34 }
    );

    drawMenuDecor(menu, menuPanel);

    Rectangle titlePlate = {
        menuPanel.x + 34.0f,
        menuPanel.y + 26.0f,
        menuPanel.width - 68.0f,
        138.0f
    };

    DrawRectangleRounded(
        titlePlate,
        0.08f, 12,
        (Color){ 8, 12, 24, 96 }
    );

    const char *title = "SPACE SHOOTER";

    float t = menu_clampf(menu->titlePulse / 2.3f, 0.0f, 1.0f);
    float grow = menu_easeOutBack(menu_smoothstep(0.0f, 0.72f, t));
    float settle = menu_smoothstep(0.72f, 1.0f, t);
    
    float titleScale = 0.62f + 0.48f * grow - 0.10f * settle;
    float fontSize = 84.0f * titleScale;
    float fontSpacing = 4.0f;
    
    Vector2 textSize = MeasureTextEx(game->font, title, fontSize, fontSpacing);
    float titleY = menuPanel.y + 36.0f + (1.0f - t) * 34.0f;
    
    Vector2 textPos = {
        centerX - textSize.x / 2.0f,
        titleY
    };
        
    drawSoftGlow((Vector2){ centerX, menuPanel.y + 82.0f }, 170.0f, (Color){ 60, 200, 255, (unsigned char)(18 + 34 * t) });
    drawSoftGlow((Vector2){ centerX - 58.0f, menuPanel.y + 78.0f }, 112.0f, (Color){ 145, 30, 230, (unsigned char)(12 + 24 * t) });
    
    if (t < 1.0f) {
        float sweep = 1.0f - t;
        DrawRing(
                 (Vector2){ centerX, menuPanel.y + 82.0f },
                 26.0f + sweep * 20.0f,
                 44.0f + sweep * 56.0f,
                 12.0f,
                 348.0f,
                 72,
                 (Color){ 255, 255, 255, (unsigned char)(20 + 40 * sweep) }
                 );
    }
    
    DrawTextEx(
               game->font,
               title,
               (Vector2){ textPos.x + 2.0f, textPos.y + 3.0f },
               fontSize, fontSpacing,
               (Color){ 0, 0, 0, (unsigned char)(80 + 90 * t) }
               );
    
    DrawTextEx(
               game->font,
               title,
               textPos,
               fontSize, fontSpacing,
               (Color){ 232, 236, 246, (unsigned char)(110 + 145 * t) }
               );
    
    if (s_titleGlowTimer >= 0.0f) {
        drawTitleGlowLetters(game, title, textPos, fontSize, fontSpacing, t, s_titleGlowTimer);
    }
    
    DrawRectangleRec(
                     (Rectangle){ menuPanel.x + 122.0f, menuPanel.y + 160.0f, menuPanel.width - 244.0f, 2.0f },
                     (Color){ 60, 200, 255, 95 }
                     );
    DrawRectangleRec(
                     (Rectangle){ menuPanel.x + 164.0f, menuPanel.y + 166.0f, menuPanel.width - 328.0f, 1.0f },
                     (Color){ 145, 30, 230, 70 }
                     );
    
    const char *subtitle = "THE VOID AWAITS";
    float subSize = 18.0f;
    float subSpace = 2.0f;
    Vector2 subText = MeasureTextEx(game->font, subtitle, subSize, subSpace);
    Vector2 subPos = { centerX - subText.x / 2.0f, menuPanel.y + 178.0f };

    DrawTextEx(
        game->font,
        subtitle,
        (Vector2){ subPos.x + 1.0f, subPos.y + 1.0f },
        subSize, subSpace,
        (Color){ 10, 10, 10, 110 }
    );
    DrawTextEx(
        game->font,
        subtitle,
        subPos,
        subSize, subSpace,
        (Color){ 180, 190, 210, 255 }
    );

    Rectangle content = {
        menuPanel.x + 52.0f,
        menuPanel.y + 210.0f,
        menuPanel.width - 104.0f,
        308.0f
    };

    DrawRectangleRounded(
        (Rectangle){ content.x, content.y, content.width, content.height },
        0.06f, 12,
        (Color){ 255, 255, 255, 6 }
    );

    DrawRectangleRec(
        (Rectangle){ centerX - 1.0f, content.y + 18.0f, 2.0f, content.height - 36.0f },
        (Color){ 255, 255, 255, 16 }
    );

    bool startHover = CheckCollisionPointRec(menu->mousePos, menu->startButton.bounds);
    bool leaderHover = CheckCollisionPointRec(menu->mousePos, menu->leaderboardButton.bounds);
    bool settingHover = CheckCollisionPointRec(menu->mousePos, menu->settingButton.bounds);
    bool exitHover = CheckCollisionPointRec(menu->mousePos, menu->exitButton.bounds);

    float pulse = 0.5f + 0.5f * sinf(menu->timer * 3.0f);

    float introA = menu_smoothstep(0.15f, 0.95f, menu->titlePulse);
    float introB = menu_smoothstep(0.32f, 1.15f, menu->titlePulse);
    float introC = menu_smoothstep(0.34f, 1.18f, menu->titlePulse);
    float introD = menu_smoothstep(0.40f, 1.25f, menu->titlePulse);

    drawButton(game, &menu->startButton, startHover, (Color){ 145, 30, 230, 255 }, pulse, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_START ? 1.0f : 0.0f, introA, true, 1.00f);
    drawButton(game, &menu->leaderboardButton, leaderHover, (Color){ 60, 200, 255, 255 }, pulse + 0.34f, 0.0f, introB, false, 1.00f);
    drawButton(game, &menu->settingButton, settingHover, (Color){ 180, 140, 255, 255 }, pulse + 0.68f, 0.0f, introC, false, 1.00f);
    drawButton(game, &menu->exitButton, exitHover, (Color){ 255, 92, 128, 255 }, pulse + 1.02f, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_EXIT ? 1.0f : 0.0f, introD, true, 1.45f);

    if (menu->phase == MENU_PHASE_TRANSITION) {
        float t = menu_clampf(menu->transitionTimer / menu->transitionDuration, 0.0f, 1.0f);
        float fade = menu_smoothstep(0.0f, 1.0f, t);

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu_alpha((Color){ 0, 0, 0, 255 }, (unsigned char)(150.0f * fade)));

        float burst = 1.0f - menu_smoothstep(0.0f, 0.58f, t);
        Vector2 c = { centerX, menuPanel.y + 82.0f };

        drawSoftGlow(c, 90.0f + 170.0f * burst, menu_alpha((Color){ 80, 200, 255, 255 }, (unsigned char)(80.0f * burst)));
        DrawRing(c, 20.0f + 190.0f * burst, 28.0f + 220.0f * burst, 10.0f, 350.0f, 96, menu_alpha((Color){ 145, 30, 230, 255 }, (unsigned char)(110.0f * burst)));
        DrawCircleV(c, 6.0f + 18.0f * burst, menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)(200.0f * burst)));
    }
}
