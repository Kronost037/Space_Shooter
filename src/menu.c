#include "game.h"
#include "menu.h"

#include <math.h>

#ifndef TAU
#define TAU 6.28318530718f
#endif

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

static Color menu_lerp_color(Color a, Color b, float t) {
    t = menu_clampf(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static void drawSoftGlow(Vector2 p, float radius, Color c) {
    for (int i = 5; i >= 1; --i) {
        float t = (float)i / 5.0f;
        unsigned char a = (unsigned char)((float)c.a * t * t * 0.55f);
        DrawCircleV(p, radius * t, menu_alpha(c, a));
    }
}

static void drawGlintSweep(Rectangle r, float timer, Color accent) {
    float w = r.width * 0.28f;
    float x = r.x - w + fmodf(timer * 120.0f, r.width + w * 2.0f);

    DrawRectangleGradientH(
        (int)x,
        (int)(r.y + 2.0f),
        (int)w,
        (int)(r.height - 4.0f),
        menu_alpha((Color){ 255, 255, 255, 0 }, 0),
        menu_alpha(accent, 34)
    );
}

static void drawButton(Game *game, const Button *button, bool hovered, Color accent, float pulse, float press) {
    Rectangle r = button->bounds;

    float hoverLift = hovered ? 3.0f : 0.0f;
    float pressDrop = press * 2.0f;
    Rectangle body = {
        r.x,
        r.y - hoverLift + pressDrop,
        r.width,
        r.height
    };

    DrawRectangleRounded(
        (Rectangle){ body.x + 4.0f, body.y + 6.0f, body.width, body.height },
        0.22f, 12,
        (Color){ 0, 0, 0, 90 }
    );

    Color fillTop = hovered ? (Color){ 26, 34, 58, 230 } : (Color){ 18, 24, 42, 215 };
    Color fillBottom = hovered ? (Color){ 10, 14, 28, 220 } : (Color){ 8, 10, 20, 210 };

    DrawRectangleRounded(body, 0.22f, 12, fillTop);
    DrawRectangleRounded(
        (Rectangle){ body.x, body.y + body.height * 0.44f, body.width, body.height * 0.56f },
        0.22f, 12, fillBottom
    );

    DrawRectangleRounded(
        (Rectangle){ body.x + 2.0f, body.y + 2.0f, body.width - 4.0f, body.height - 4.0f },
        0.22f, 12,
        (Color){ 255, 255, 255, hovered ? 18 : 10 }
    );

    DrawRectangleLinesEx(
        body,
        2.0f,
        hovered ? menu_alpha((Color){ 120, 220, 255, 255 }, 180) : menu_alpha((Color){ 255, 255, 255, 255 }, 58)
    );

    DrawRectangleRec(
        (Rectangle){ body.x + 12.0f, body.y + 10.0f, 5.0f, body.height - 20.0f },
        accent
    );

    DrawRectangleRec(
        (Rectangle){ body.x + 10.0f, body.y + 6.0f, body.width - 20.0f, 2.0f },
        menu_alpha((Color){ 255, 255, 255, 255 }, hovered ? 34 : 18)
    );

    drawGlintSweep(body, game->menu->timer + pulse, accent);

    if (hovered) {
        drawSoftGlow(
            (Vector2){ body.x + body.width * 0.5f, body.y + body.height * 0.5f },
            body.width * 0.52f,
            menu_alpha(accent, 24)
        );
    }

    float fontSize = hovered ? 44.0f : 42.0f;
    float fontSpacing = 2.0f;
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
        (Color){ 0, 0, 0, 120 }
    );

    DrawTextEx(
        game->font,
        button->label,
        textPos,
        fontSize, fontSpacing,
        hovered ? (Color){ 245, 248, 255, 255 } : (Color){ 220, 225, 235, 255 }
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
        SCREEN_WIDTH * 0.5f - 290.0f,
        180.0f,
        580.0f,
        470.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    game->menu->startButton.bounds = (Rectangle){
        centerX - 108.0f,
        menuPanel.y + 210.0f,
        216.0f,
        54.0f
    };

    game->menu->leaderboardButton.bounds = (Rectangle){
        centerX - 108.0f,
        menuPanel.y + 282.0f,
        216.0f,
        54.0f
    };

    game->menu->exitButton.bounds = (Rectangle){
        centerX - 108.0f,
        menuPanel.y + 354.0f,
        216.0f,
        54.0f
    };
}

void initializeMenu(Menu *menu) {
    *menu = (Menu){0};

    menu->bg_song = LoadMusicStream("src/Assets/menu.mp3");
    menu->hover_sfx = LoadSound("src/Assets/menu_button_hover.mp3");
    menu->click_sfx = LoadSound("src/Assets/menu_button_click.mp3");

    menu->mousePos = (Vector2){ 0.0f, 0.0f };
    menu->timer = 0.0f;
    menu->titlePulse = 0.0f;

    menu->transitionTimer = 0.0f;
    menu->transitionDuration = 0.85f;
    menu->phase = MENU_PHASE_IDLE;
    menu->pendingAction = MENU_ACTION_NONE;

    menu->hoveredButton = -1;
    menu->hoverPulse = 0.0f;
    menu->clickFlash = 0.0f;
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

void runMenuPhysics(Game *game) {
    Menu *menu = game->menu;

    UpdateMusicStream(menu->bg_song);

    float dt = GetFrameTime();
    menu->timer += dt;
    menu->titlePulse += dt * 1.15f;

    if (menu->timer > 1000.0f) menu->timer = 0.0f;

    if (menu->hoverPulse > 0.0f) {
        menu->hoverPulse -= dt * 2.4f;
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
    bool exitHover = CheckCollisionPointRec(menu->mousePos, menu->exitButton.bounds);

    int hovered = startHover ? 0 : leaderHover ? 1 : exitHover ? 2 : -1;

    if (hovered != menu->hoveredButton) {
        if (hovered != -1) PlaySound(menu->hover_sfx);
        menu->hoveredButton = hovered;
        menu->hoverPulse = 1.0f;
    }

    if (menu->phase == MENU_PHASE_IDLE) {
        if (startHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_START);
        } else if (leaderHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            startTransition(menu, MENU_ACTION_LEADERBOARD);
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
    float cy = panel.y + 92.0f;

    drawSoftGlow((Vector2){ cx, cy }, 120.0f, (Color){ 80, 170, 255, 24 });
    drawSoftGlow((Vector2){ cx - 140.0f, cy - 12.0f }, 80.0f, (Color){ 140, 70, 220, 18 });
    drawSoftGlow((Vector2){ cx + 150.0f, cy + 12.0f }, 72.0f, (Color){ 60, 200, 255, 14 });

    DrawRing((Vector2){ cx, cy + 6.0f }, 36.0f, 58.0f, 20.0f, 340.0f, 64, (Color){ 60, 200, 255, 26 });
    DrawRing((Vector2){ cx, cy + 6.0f }, 22.0f, 32.0f, 10.0f, 350.0f, 48, (Color){ 145, 30, 230, 18 });

    for (int i = 0; i < 5; ++i) {
        float a = t * 0.8f + (float)i * 1.4f;
        Vector2 p = {
            cx + cosf(a) * (88.0f + i * 9.0f),
            cy + sinf(a * 1.13f) * (22.0f + i * 2.0f)
        };
        Color c = (i % 2 == 0) ? (Color){ 60, 200, 255, 255 } : (Color){ 145, 30, 230, 255 };
        DrawCircleV(p, 2.0f + (float)(i % 3), menu_alpha(c, 90));
    }

    DrawRectangleRec(
        (Rectangle){ panel.x + 18.0f, panel.y + 16.0f, panel.width - 36.0f, 2.0f },
        (Color){ 60, 200, 255, 140 }
    );
    DrawRectangleRec(
        (Rectangle){ panel.x + 18.0f, panel.y + 20.0f, panel.width - 120.0f, 1.0f },
        (Color){ 145, 30, 230, 100 }
    );
}

void drawMenu(Game *game) {
    Menu *menu = game->menu;

    drawBackground(SCREEN_WIDTH, SCREEN_HEIGHT, menu->timer);

    Rectangle menuPanel = {
        SCREEN_WIDTH * 0.5f - 290.0f,
        180.0f,
        580.0f,
        470.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    DrawRectangleRec(
        (Rectangle){ 0, menuPanel.y + 30.0f, SCREEN_WIDTH, 2.0f },
        (Color){ 145, 30, 230, 30 }
    );
    DrawRectangleRec(
        (Rectangle){ 0, menuPanel.y + 33.0f, SCREEN_WIDTH, 1.0f },
        (Color){ 60, 200, 255, 18 }
    );

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x - 7.0f, menuPanel.y - 7.0f, menuPanel.width + 14.0f, menuPanel.height + 14.0f },
        0.08f, 16,
        (Color){ 145, 30, 230, 18 }
    );

    DrawRectangleRounded(menuPanel, 0.08f, 16, (Color){ 12, 16, 30, 220 });

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 2.0f, menuPanel.width - 4.0f, menuPanel.height * 0.34f },
        0.08f, 16,
        (Color){ 255, 255, 255, 12 }
    );

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + menuPanel.height * 0.58f, menuPanel.width - 4.0f, menuPanel.height * 0.40f },
        0.08f, 16,
        (Color){ 0, 0, 0, 28 }
    );

    DrawRectangleLinesEx(menuPanel, 2.0f, (Color){ 255, 255, 255, 48 });
    DrawRectangleLinesEx(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 2.0f, menuPanel.width - 4.0f, menuPanel.height - 4.0f },
        1.0f, (Color){ 145, 30, 230, 34 }
    );

    drawMenuDecor(menu, menuPanel);

    const char *title = "SPACE SHOOTER";
    float titleBreathe = 1.0f + 0.028f * sinf(menu->timer * 1.35f);
    float titleScale = titleBreathe;
    float fontSize = 82.0f * titleScale;
    float fontSpacing = 4.0f;

    Vector2 textSize = MeasureTextEx(game->font, title, fontSize, fontSpacing);
    Vector2 textPos = {
        centerX - textSize.x / 2.0f,
        menuPanel.y + 34.0f
    };

    drawSoftGlow(
        (Vector2){ centerX, menuPanel.y + 76.0f },
        150.0f,
        (Color){ 60, 200, 255, 20 }
    );
    drawSoftGlow(
        (Vector2){ centerX - 52.0f, menuPanel.y + 76.0f },
        96.0f,
        (Color){ 145, 30, 230, 16 }
    );

    DrawTextEx(
        game->font,
        title,
        (Vector2){ textPos.x + 2.0f, textPos.y + 3.0f },
        fontSize, fontSpacing,
        (Color){ 0, 0, 0, 120 }
    );
    DrawTextEx(
        game->font,
        title,
        textPos,
        fontSize, fontSpacing,
        (Color){ 230, 235, 245, 255 }
    );

    float lineW = textSize.x + 36.0f;
    float lineX = centerX - lineW / 2.0f;

    DrawRectangleRec(
        (Rectangle){ lineX, textPos.y - 10.0f, lineW, 2.0f },
        (Color){ 145, 30, 230, 180 }
    );
    DrawRectangleRec(
        (Rectangle){ lineX + 26.0f, textPos.y + textSize.y + 10.0f, lineW - 52.0f, 1.0f },
        (Color){ 60, 200, 255, 95 }
    );

    float shineW = lineW * 0.22f;
    float shineX = lineX - shineW + fmodf(menu->timer * 120.0f, lineW + shineW * 2.0f);
    DrawRectangleGradientH(
        (int)shineX,
        (int)(textPos.y - 8.0f),
        (int)shineW,
        (int)(textSize.y + 18.0f),
        (Color){ 255, 255, 255, 0 },
        (Color){ 255, 255, 255, 22 }
    );

    const char *subtitle = "ARE YOU READY?";
    float subSize = 20.0f;
    float subSpace = 2.0f;
    Vector2 subText = MeasureTextEx(game->font, subtitle, subSize, subSpace);
    Vector2 subPos = { centerX - subText.x / 2.0f, menuPanel.y + 146.0f };

    DrawTextEx(
        game->font,
        subtitle,
        (Vector2){ subPos.x + 1.0f, subPos.y + 1.0f },
        subSize, subSpace,
        (Color){ 0, 0, 0, 110 }
    );
    DrawTextEx(
        game->font,
        subtitle,
        subPos,
        subSize, subSpace,
        (Color){ 180, 190, 210, 255 }
    );

    bool startHover = CheckCollisionPointRec(menu->mousePos, menu->startButton.bounds);
    bool leaderHover = CheckCollisionPointRec(menu->mousePos, menu->leaderboardButton.bounds);
    bool exitHover = CheckCollisionPointRec(menu->mousePos, menu->exitButton.bounds);

    float pulse = 0.5f + 0.5f * sinf(menu->timer * 3.0f);

    drawButton(game, &menu->startButton, startHover, (Color){ 145, 30, 230, 255 }, pulse, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_START ? 1.0f : 0.0f);
    drawButton(game, &menu->leaderboardButton, leaderHover, (Color){ 60, 200, 255, 255 }, pulse + 0.34f, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_LEADERBOARD ? 1.0f : 0.0f);
    drawButton(game, &menu->exitButton, exitHover, (Color){ 255, 120, 120, 255 }, pulse + 0.68f, menu->phase == MENU_PHASE_TRANSITION && menu->pendingAction == MENU_ACTION_EXIT ? 1.0f : 0.0f);

    DrawRectangleRec(
        (Rectangle){ menuPanel.x + 20.0f, menuPanel.y + menuPanel.height - 34.0f, menuPanel.width - 40.0f, 1.0f },
        (Color){ 255, 255, 255, 22 }
    );

    DrawTextEx(
        game->font,
        "ENTER THE VOID",
        (Vector2){ centerX - MeasureTextEx(game->font, "ENTER THE VOID", 18.0f, 2.0f).x / 2.0f, menuPanel.y + 418.0f },
        18.0f, 2.0f,
        (Color){ 160, 170, 190, 200 }
    );

    if (menu->phase == MENU_PHASE_TRANSITION) {
        float t = menu_clampf(menu->transitionTimer / menu->transitionDuration, 0.0f, 1.0f);
        float fade = menu_smoothstep(0.0f, 1.0f, t);

        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, menu_alpha((Color){ 0, 0, 0, 255 }, (unsigned char)(180.0f * fade)));

        float burst = 1.0f - menu_smoothstep(0.0f, 0.45f, t);
        Vector2 c = { centerX, menuPanel.y + 92.0f };
        drawSoftGlow(c, 90.0f + 180.0f * burst, menu_alpha((Color){ 80, 200, 255, 255 }, (unsigned char)(90.0f * burst)));
        DrawRing(c, 20.0f + 200.0f * burst, 28.0f + 240.0f * burst, 10.0f, 350.0f, 96, menu_alpha((Color){ 145, 30, 230, 255 }, (unsigned char)(120.0f * burst)));
        DrawCircleV(c, 6.0f + 22.0f * burst, menu_alpha((Color){ 255, 255, 255, 255 }, (unsigned char)(220.0f * burst)));
    }
}
