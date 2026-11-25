#include "menu.h"
#include "flight/flight.h"
#include "flight/mission.h"
#include "ui/widgets.h"

static const struct { const char *label; MenuAction action; } choices[] = {
    {"Play", MENU_ACTION_START},
    {"Records", MENU_ACTION_LEADERBOARD},
    {"Settings", MENU_ACTION_SETTING},
    {"Quit", MENU_ACTION_EXIT},
};

static Rectangle choiceBounds(int index) {
    return (Rectangle){104, 415 + (float)index * 67, 350, 59};
}

void initializeMenu(Menu *menu) {
    *menu = (Menu){0};
    menu->bg_song = LoadMusicStream("src/Assets/menu.mp3");
    menu->hover_sfx = LoadSound("src/Assets/menu_button_hover.mp3");
    menu->click_sfx = LoadSound("src/Assets/menu_button_click.mp3");
}

void runMenuPhysics(Game *game) {
    Menu *menu = game->menu;
    float dt = fminf(GetFrameTime(), 0.1f);
    menu->time += dt;
    bool activity = Vector2LengthSqr(GetMouseDelta()) > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                    IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_ENTER) ||
                    IsKeyPressed(KEY_KP_ENTER);
    menu->idleTime = activity || menu->pending ? 0 : menu->idleTime + dt;
    if (menu->pending) {
        menu->transition += dt;
        return;
    }
    int focus = menu->focus;
    if (IsKeyPressed(KEY_DOWN)) focus = (focus + 1) % 4;
    if (IsKeyPressed(KEY_UP)) focus = (focus + 3) % 4;
    bool activate = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    bool moved = Vector2LengthSqr(GetMouseDelta()) > 0;
    for (int i = 0; i < 4; ++i) {
        Rectangle row = choiceBounds(i);
        if (moved && CheckCollisionPointRec(GetMousePosition(), row)) focus = i;
        if (uiClicked(row)) { focus = i; activate = true; }
    }
    if (focus != menu->focus) PlaySound(menu->hover_sfx);
    menu->focus = focus;
    if (activate) {
        menu->pending = choices[focus].action;
        PlaySound(menu->click_sfx);
    }
}

bool menuActionReady(const Menu *menu) {
    return menu->pending && menu->transition >= (menu->pending == MENU_ACTION_START ? 0.48f : 0.16f);
}

MenuAction menuConsumeAction(Menu *menu) {
    MenuAction action = menu->pending;
    menu->pending = MENU_ACTION_NONE;
    menu->transition = 0;
    return action;
}

static void drawPatrol(const Game *game, float time) {
    // A distant transport crosses the opening before the pilot in the foreground.
    float transit = fmodf(time, 38.0f) / 38.0f;
    Vector2 freighter = {790 + 835 * transit, 290 - 85 * transit};
    drawTransport(game, freighter, direction(-0.10f), 53, 0.18f);
    float phase = time * 0.51f;
    bool departing = game->menu->pending == MENU_ACTION_START;
    float launch = departing ? Clamp(game->menu->transition / 0.48f, 0, 1) : 0;
    Vector2 nose = direction(-0.49f + 0.035f * sinf(phase));
    Vector2 center = {1005 + 16 * sinf(phase * 0.6f), 462 + 8 * cosf(phase)};
    center = Vector2Add(center, Vector2Scale(nose, 380 * launch * launch));
    float size = 275 - 84 * launch;
    Vector2 side = {-nose.y, nose.x};
    for (int i = -1; i <= 1; i += 2) {
        Vector2 nozzle = Vector2Add(Vector2Subtract(center, Vector2Scale(nose, size * 0.33f)),
                                    Vector2Scale(side, i * size * 0.13f));
        float wake = 34 + 6 * sinf(phase * 7 + i) + 110 * launch;
        for (int j = 5; j >= 0; --j) {
            float u = j / 5.0f;
            Vector2 at = Vector2Subtract(nozzle, Vector2Scale(nose, u * wake));
            DrawCircleGradient(at, (16 - 8 * u), Fade(UI_ICE, 0.18f * (1 - u)), Fade(UI_ICE, 0));
        }
    }
    drawShip(game->pilotTexture, center, nose, size, (Color){222,222,211,255}, UI_ICE);
}

void drawMenu(Game *game) {
    float t = game->menu->time;
    uiBackdrop(game, t);
    // Confine the brief shower to the open sky above the pilot.
    float shower = fmodf(game->menu->idleTime, 64.0f) - 25.0f;
    for (int i = 0; i < 3; ++i) {
        float progress = (shower - i * 0.72f) / 0.8f;
        if (progress <= 0 || progress >= 1) continue;
        Vector2 head = {878 + progress * 175 + i * 26, 209 + progress * 88 - i * 13};
        Vector2 tail = Vector2Subtract(head, (Vector2){36,18});
        float light = sinf(progress * PI);
        DrawLineEx(tail, head, 1.7f, Fade(UI_PAPER, light * 0.62f));
        DrawCircleV(head, 2, Fade(UI_PAPER, light));
    }
    drawPatrol(game, t);
    uiTitle(game, "Sector", 99, 158, 97);
    uiTitle(game, "Siege", 94, 231, 148);
    for (int i = 0; i < 4; ++i) {
        Rectangle row = choiceBounds(i);
        uiAction(game, row, choices[i].label, i == game->menu->focus);
    }
    if (game->menu->pending)
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(UI_INK, Clamp(game->menu->pending == MENU_ACTION_START ? (game->menu->transition - 0.28f) / 0.20f : game->menu->transition / 0.16f, 0, 1)));
}
