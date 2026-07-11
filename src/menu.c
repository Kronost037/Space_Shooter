#include "game.h"
#include "menu.h"
#include "background.h"

#ifndef TAU
#define TAU 6.28318530718f
#endif

#define TITLE_INTRO_DURATION 2.35f
#define TITLE_GLOW_DELAY 0.62f
#define TITLE_GLOW_DURATION 1.30f
#define MENU_TRANSITION_DURATION 1.42f

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

static float menu_easeOutCubic(float x) {
    float t = 1.0f - menu_clampf(x, 0.0f, 1.0f);
    return 1.0f - t * t * t;
}

static Color menu_alpha(Color c, unsigned char a) {
    c.a = a;
    return c;
}

static void drawSoftGlow(Vector2 p, float radius, Color c) {
    for (int i = 6; i >= 1; --i) {
        float t = (float)i / 6.0f;
        unsigned char a = (unsigned char)((float)c.a * t * t * 0.48f);
        DrawCircleV(p, radius * t, menu_alpha(c, a));
    }
}

static void drawGlintSweep(Rectangle r, float timer, Color accent, float intensity) {
    float w = r.width * 0.28f;
    float x = r.x - w + fmodf(timer * 74.0f, r.width + w * 2.0f);

    DrawRectangleGradientH(
        (int)x,
        (int)(r.y + 2.0f),
        (int)w,
        (int)(r.height - 4.0f),
        menu_alpha((Color){ 255, 255, 255, 0 }, 0),
        menu_alpha(accent, (unsigned char)(26.0f * intensity))
   );
}

static void drawButton(Game *game, const Button *button, bool hovered, Color accent, float pulse, float press, float appear, bool primary, float glowBoost) {
    Rectangle r = button->bounds;
    appear = menu_clampf(appear, 0.0f, 1.0f);

    float introLift = (1.0f - appear) * 12.0f;
    float hoverLift = hovered ? (primary ? 3.5f : 2.5f) : 0.0f;
    float pressDrop = press * 2.2f;

    float hoverScale = hovered ? (1.0f + 0.014f * game->menu->hoverPulse) : 1.0f;
    float w = r.width * hoverScale;
    float h = r.height * hoverScale;

    Rectangle body = {
        r.x + (r.width - w) * 0.5f,
        r.y - hoverLift + pressDrop + introLift + (r.height - h) * 0.5f,
        w,
        h
    };

    unsigned char appearA = (unsigned char)(255.0f * appear);
    unsigned char shadowA = (unsigned char)(72.0f * appear);

    DrawRectangleRounded(
        (Rectangle){ body.x + 4.0f, body.y + 7.0f, body.width, body.height },
        0.22f, 12,
        (Color){ 0, 0, 0, shadowA }
    );

    Color top = hovered
        ? (Color){ 32, 44, 74, (unsigned char)(238.0f * appear) }
        : (Color){ 22, 30, 50, (unsigned char)(232.0f * appear) };

    Color bottom = hovered
        ? (Color){ 12, 16, 30, (unsigned char)(232.0f * appear) }
        : (Color){ 10, 12, 24, (unsigned char)(224.0f * appear) };

    DrawRectangleRounded(body, 0.22f, 12, top);
    DrawRectangleRounded(
        (Rectangle){ body.x, body.y + body.height * 0.44f, body.width, body.height * 0.56f },
        0.22f, 12, bottom
    );

    DrawRectangleRounded(
        (Rectangle){ body.x + 2.0f, body.y + 2.0f, body.width - 4.0f, body.height - 4.0f },
        0.22f, 12,
        (Color){ 255, 255, 255, (unsigned char)(14.0f * appear) }
    );

    DrawRectangleLinesEx(
        body,
        2.0f,
        hovered
            ? menu_alpha((Color){ 110, 215, 255, 255 }, (unsigned char)(180.0f * appear))
            : menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)(58.0f * appear))
    );

    DrawRectangleRec(
        (Rectangle){ body.x + 11.0f, body.y + 9.0f, 5.0f, body.height - 18.0f },
        menu_alpha(accent, appearA)
    );

    DrawRectangleRec(
        (Rectangle){ body.x + 10.0f, body.y + 6.0f, body.width - 20.0f, 2.0f },
        menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)((hovered ? 30.0f : 16.0f) * appear))
    );

    drawGlintSweep(body, game->menu->timer + pulse, accent, appear);

    if (hovered) {
        drawSoftGlow(
            (Vector2){ body.x + body.width * 0.5f, body.y + body.height * 0.5f },
            body.width * 0.48f * glowBoost,
            menu_alpha(accent, (unsigned char)(22.0f * appear))
        );
    }

    float fontSize = primary ? (hovered ? 40.0f : 38.0f) : (hovered ? 23.0f : 21.0f);
    float fontSpacing = primary ? 2.0f : 1.3f;
    Vector2 textSize = MeasureTextEx(game->font, button->label, fontSize, fontSpacing);

    Vector2 textPos = {
        body.x + body.width * 0.5f - textSize.x * 0.5f,
        body.y + body.height * 0.5f - textSize.y * 0.5f - 1.0f
    };

    DrawTextEx(
        game->font,
        button->label,
        (Vector2){ textPos.x + 1.5f, textPos.y + 1.5f },
        fontSize, fontSpacing,
        menu_alpha((Color){ 0, 0, 0, 126 }, appearA)
    );

    DrawTextEx(
        game->font,
        button->label,
        textPos,
        fontSize, fontSpacing,
        hovered
            ? menu_alpha((Color){ 247, 250, 255, 255 }, appearA)
            : menu_alpha((Color){ 232, 236, 244, 255 }, appearA)
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

    Rectangle content = {
        menuPanel.x + 52.0f,
        menuPanel.y + 226.0f,
        menuPanel.width - 104.0f,
        302.0f
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
    menu->transitionDuration = MENU_TRANSITION_DURATION;
    menu->phase = MENU_PHASE_IDLE;
    menu->pendingAction = MENU_ACTION_NONE;

    menu->hoveredButton = -1;
    menu->hoverPulse = 0.0f;
    menu->clickFlash = 0.0f;

    menu->startButton.label = "START";
    menu->leaderboardButton.label = "LEADERBOARD";
    menu->settingButton.label = "SETTINGS";
    menu->exitButton.label = "EXIT";

    if (s_titleIntroPlayed) {
        s_titleGlowTimer = TITLE_GLOW_DELAY + TITLE_GLOW_DURATION + 1.0f;
    }
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

static void drawTitleGlowSweep(Game *game, const char *text, Vector2 pos, float fontSize, float spacing, float sweepTimer) {
    float totalWidth = MeasureTextEx(game->font, text, fontSize, spacing).x;
    if (totalWidth <= 0.0f) return;

    float delay = TITLE_GLOW_DELAY;
    float duration = TITLE_GLOW_DURATION;
    if (duration <= 0.0f) duration = 0.01f;

    float whitePhase = menu_clampf(sweepTimer / delay, 0.0f, 1.0f);
    float bluePhase = menu_clampf((sweepTimer - delay) / duration, 0.0f, 1.0f);

    if (sweepTimer < 0.0f) return;

    float sweepX = pos.x + totalWidth * (0.02f + 0.96f * bluePhase);
    float sweepBand = totalWidth * 0.18f;

    BeginBlendMode(BLEND_ADDITIVE);

    float cursorX = pos.x;

    for (const char *p = text; *p; ++p) {
        char ch[2] = { *p, '\0' };
        Vector2 cs = MeasureTextEx(game->font, ch, fontSize, spacing);
        float charW = cs.x;
        float charCenter = cursorX + charW * 0.5f;

        float dist = fabsf(charCenter - sweepX);
        float band = sweepBand + charW * 0.55f;

        float blueGlow = 0.0f;
        if (bluePhase > 0.0f) {
            blueGlow = 1.0f - menu_clampf(dist / band, 0.0f, 1.0f);
            blueGlow = blueGlow * blueGlow * (3.0f - 2.0f * blueGlow);
        }

        float whiteGlow = 0.55f + 0.45f * (1.0f - whitePhase * 0.35f);

        unsigned char whiteA = (unsigned char)(30.0f * whiteGlow);
        unsigned char blueA   = (unsigned char)(198.0f * blueGlow);

       
        DrawTextEx(game->font, ch, (Vector2){ cursorX - 2.0f, pos.y - 1.0f }, fontSize + 3.0f, spacing, (Color){ 55, 55, 255, whiteA });
        DrawTextEx(game->font, ch, (Vector2){ cursorX + 2.0f, pos.y + 1.0f }, fontSize + 3.0f, spacing, (Color){ 55, 55, 255, whiteA });
        DrawTextEx(game->font, ch, (Vector2){ cursorX, pos.y - 2.0f }, fontSize + 4.0f, spacing, (Color){ 55, 55, 255, (unsigned char)(whiteA * 0.8f) });
        DrawTextEx(game->font, ch, (Vector2){ cursorX, pos.y + 2.0f }, fontSize + 4.0f, spacing, (Color){ 55, 55, 255, (unsigned char)(whiteA * 0.8f) });

        

        if (blueGlow > 0.0f) {
            DrawTextEx(game->font, ch, (Vector2){ cursorX - 2.0f, pos.y }, fontSize + 3.0f, spacing, (Color){ 70, 210, 255, blueA });
            DrawTextEx(game->font, ch, (Vector2){ cursorX + 2.0f, pos.y }, fontSize + 3.0f, spacing, (Color){ 130, 245, 255, blueA });
            DrawTextEx(game->font, ch, (Vector2){ cursorX, pos.y - 1.0f }, fontSize + 2.0f, spacing, (Color){ 100, 230, 255, (unsigned char)(blueA * 0.75f) });
        }

        
        DrawTextEx(game->font, ch, (Vector2){ cursorX, pos.y }, fontSize, spacing, (Color){ 245, 250, 255, 255 });

        cursorX += charW + spacing;
    }

    EndBlendMode();
}


void runMenuPhysics(Game *game) {
    Menu *menu = game->menu;

    UpdateMusicStream(menu->bg_song);

    float dt = GetFrameTime();
    menu->timer += dt;
    if (menu->timer > 1000.0f) {
        menu->timer = 0.0f;
    }

    if (menu->titlePulse < 1.0f) {
        menu->titlePulse += dt / TITLE_INTRO_DURATION;
        if (menu->titlePulse >= 1.0f) {
            menu->titlePulse = 1.0f;
            s_titleIntroPlayed = true;
            if (s_titleGlowTimer < 0.0f) {
                s_titleGlowTimer = 0.0f;
            }
        }
    } else if (s_titleGlowTimer >= 0.0f) {
        s_titleGlowTimer += dt;
        if (s_titleGlowTimer > TITLE_GLOW_DELAY + TITLE_GLOW_DURATION + 0.12f) {
            s_titleGlowTimer = TITLE_GLOW_DELAY + TITLE_GLOW_DURATION + 0.12f;
        }
    }

    if (menu->hoverPulse > 0.0f) {
        menu->hoverPulse -= dt * 1.55f;
        if (menu->hoverPulse < 0.0f) {
            menu->hoverPulse = 0.0f;
        }
    }

    if (menu->clickFlash > 0.0f) {
        menu->clickFlash -= dt * 2.0f;
        if (menu->clickFlash < 0.0f) {
            menu->clickFlash = 0.0f;
        }
    }

    menu->mousePos = GetMousePosition();

    updateMenuLayout(game);

    bool startHover = CheckCollisionPointRec(menu->mousePos, menu->startButton.bounds);
    bool leaderHover = CheckCollisionPointRec(menu->mousePos, menu->leaderboardButton.bounds);
    bool settingHover = CheckCollisionPointRec(menu->mousePos, menu->settingButton.bounds);
    bool exitHover = CheckCollisionPointRec(menu->mousePos, menu->exitButton.bounds);

    int hovered = startHover ? 0 : leaderHover ? 1 : settingHover ? 2 : exitHover ? 3 : -1;

    if (hovered != menu->hoveredButton) {
        if (hovered != -1) {
            PlaySound(menu->hover_sfx);
        }
        menu->hoveredButton = hovered;
        menu->hoverPulse = 1.0f;
    }

    if (menu->phase == MENU_PHASE_IDLE) {
        if (startHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_START);
        } else if (exitHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_EXIT);
        } else if (leaderHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_LEADERBOARD);
        } else if (settingHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_SETTING);
        }
    } else if (menu->phase == MENU_PHASE_TRANSITION) {
        menu->transitionTimer += dt;
    }
}

static void drawMenuDecor(const Menu *menu, Rectangle panel) {
    float t = menu->timer;
    float cx = panel.x + panel.width * 0.5f;
    float titleY = panel.y + 72.0f;

    drawSoftGlow((Vector2){ cx, titleY + 8.0f }, 126.0f, (Color){ 80, 170, 255, 24 });
    drawSoftGlow((Vector2){ cx - 140.0f, titleY + 2.0f }, 84.0f, (Color){ 140, 70, 220, 18 });
    drawSoftGlow((Vector2){ cx + 148.0f, titleY + 10.0f }, 76.0f, (Color){ 60, 200, 255, 16 });

    DrawRing((Vector2){ cx, titleY + 6.0f }, 40.0f, 68.0f, 22.0f, 340.0f, 64, (Color){ 60, 200, 255, 22 });
    DrawRing((Vector2){ cx, titleY + 6.0f }, 24.0f, 35.0f, 10.0f, 350.0f, 48, (Color){ 145, 30, 230, 14 });

    for (int i = 0; i < 5; ++i) {
        float a = t * 0.58f + (float)i * 1.52f;
        Vector2 p = {
            cx + cosf(a) * (96.0f + i * 11.0f),
            titleY + sinf(a * 1.13f) * (22.0f + i * 2.0f)
        };

        Color c = (i % 2 == 0)
            ? (Color){ 60, 200, 255, 255 }
            : (Color){ 145, 30, 230, 255 };

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

    float titleT = menu_clampf(menu->titlePulse, 0.0f, 1.0f);
    float rise = menu_easeOutCubic(menu_smoothstep(0.0f, 0.85f, titleT));
    float settle = menu_smoothstep(0.86f, 1.0f, titleT);
    float titleScale = 0.60f + 0.44f * rise - 0.04f * settle;
    float fontSize = 86.0f * titleScale;
    float fontSpacing = 4.0f;

    Vector2 textSize = MeasureTextEx(game->font, title, fontSize, fontSpacing);
    float titleY = menuPanel.y + 34.0f + (1.0f - titleT) * 28.0f;

    Vector2 textPos = {
        centerX - textSize.x * 0.5f,
        titleY
    };

    drawSoftGlow((Vector2){ centerX, menuPanel.y + 82.0f }, 176.0f, (Color){ 60, 200, 255, (unsigned char)(20 + 34 * titleT) });
    drawSoftGlow((Vector2){ centerX - 58.0f, menuPanel.y + 78.0f }, 116.0f, (Color){ 145, 30, 230, (unsigned char)(14 + 24 * titleT) });

    if (titleT < 1.0f) {
        float sweep = 1.0f - titleT;
        DrawRing(
            (Vector2){ centerX, menuPanel.y + 82.0f },
            26.0f + sweep * 18.0f,
            44.0f + sweep * 54.0f,
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
        (Color){ 0, 0, 0, (unsigned char)(82 + 86 * titleT) }
    );

    DrawTextEx(
        game->font,
        title,
        textPos,
        fontSize, fontSpacing,
        (Color){ 238, 242, 250, (unsigned char)(128 + 120 * titleT) }
    );

    if (s_titleGlowTimer >= 0.0f && s_titleGlowTimer <= TITLE_GLOW_DELAY + TITLE_GLOW_DURATION) {
        drawTitleGlowSweep(game, title, textPos, fontSize, fontSpacing, s_titleGlowTimer);
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
    float subSpace = 3.0f;
    Vector2 subText = MeasureTextEx(game->font, subtitle, subSize, subSpace);
    Vector2 subPos = { centerX - subText.x * 0.5f, menuPanel.y + 178.0f };

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
        (Color){ 206, 214, 228, 255 }
    );

    Rectangle content = {
        menuPanel.x + 52.0f,
        menuPanel.y + 226.0f,
        menuPanel.width - 104.0f,
        302.0f
    };

    DrawRectangleRounded(
        (Rectangle){ content.x, content.y, content.width, content.height },
        0.06f, 12,
        (Color){ 255, 255, 255, 6 }
    );


    bool startHover = CheckCollisionPointRec(menu->mousePos, menu->startButton.bounds);
    bool leaderHover = CheckCollisionPointRec(menu->mousePos, menu->leaderboardButton.bounds);
    bool settingHover = CheckCollisionPointRec(menu->mousePos, menu->settingButton.bounds);
    bool exitHover = CheckCollisionPointRec(menu->mousePos, menu->exitButton.bounds);

    float pulse = 0.5f + 0.5f * sinf(menu->timer * 2.1f);

    float introA = menu_smoothstep(0.10f, 0.98f, menu->titlePulse);
    float introB = menu_smoothstep(0.26f, 1.06f, menu->titlePulse);
    float introC = menu_smoothstep(0.36f, 1.14f, menu->titlePulse);
    float introD = menu_smoothstep(0.48f, 1.22f, menu->titlePulse);

    drawButton(game, &menu->startButton, startHover, (Color){ 145, 30, 230, 255 }, pulse, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_START ? 1.0f : 0.0f, introA, true, 1.00f);

    drawButton(game, &menu->leaderboardButton, leaderHover, (Color){ 60, 200, 255, 255 }, pulse + 0.34f, 0.0f, introB, false, 1.00f);

    drawButton(game, &menu->settingButton, settingHover, (Color){ 180, 140, 255, 255 }, pulse + 0.68f, 0.0f, introC, false, 1.00f);

    drawButton(game, &menu->exitButton, exitHover, (Color){ 255, 92, 128, 255 }, pulse + 1.02f, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_EXIT ? 1.0f : 0.0f, introD, true, 1.45f);


    if (menu->phase == MENU_PHASE_TRANSITION) {
        float t = menu_clampf(menu->transitionTimer / menu->transitionDuration, 0.0f, 1.0f);
        float fade = menu_smoothstep(0.0f, 1.0f, t);

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu_alpha((Color){ 0, 0, 0, 255 }, (unsigned char)(144.0f * fade)));

        float burst = 1.0f - menu_smoothstep(0.0f, 0.58f, t);
        Vector2 c = { centerX, menuPanel.y + 82.0f };

        drawSoftGlow(c, 90.0f + 170.0f * burst, menu_alpha((Color){ 80, 200, 255, 255 }, (unsigned char)(76.0f * burst)));
        DrawRing(c, 20.0f + 190.0f * burst, 28.0f + 220.0f * burst, 10.0f, 350.0f, 96, menu_alpha((Color){ 145, 30, 230, 255 }, (unsigned char)(108.0f * burst)));
        DrawCircleV(c, 6.0f + 18.0f * burst, menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)(200.0f * burst)));
    }
}
