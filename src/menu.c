#include "menu.h"
#include "ui/widgets.h"

static const struct { const char *label; MenuAction action; } choices[] = {
    {"Play", MENU_ACTION_START},
    {"Records", MENU_ACTION_LEADERBOARD},
    {"Settings", MENU_ACTION_SETTING},
    {"Quit", MENU_ACTION_EXIT},
};

static Rectangle choiceBounds(int index) {
    return (Rectangle){94, 372 + (float)index * 71, 406, 57};
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
    return menu->pending && menu->transition >= 0.16f;
}

MenuAction menuConsumeAction(Menu *menu) {
    MenuAction action = menu->pending;
    menu->pending = MENU_ACTION_NONE;
    menu->transition = 0;
    return action;
}

static void drawPatrol(const Game *game, float time) {
    Vector2 center = {1040, 428 + sinf(time * 0.85f) * 7};
    for (int i = 0; i < 3; ++i) {
        float angle = time * (i % 2 ? -5 : 6) + i * 92;
        float r = 195 + i * 18;
        DrawRing(center, r, r + 2, angle, angle + 102, 64, Fade(UI_ICE, 0.24f));
        DrawRing(center, r, r + 2, angle + 110, angle + 120, 10, UI_AMBER);
    }
    Texture2D ship = game->pilotTexture;
    if (IsTextureValid(ship)) {
        float size = 380, h = size * ship.height / ship.width;
        DrawTexturePro(ship, (Rectangle){0,0,(float)ship.width,(float)ship.height},
                       (Rectangle){center.x,center.y,size,h}, (Vector2){size / 2,h / 2},
                       25 + 3 * sinf(time * 0.42f), WHITE);
    }
}

void drawMenu(Game *game) {
    float t = game->menu->time;
    uiBackdrop(game, t);
    drawPatrol(game, t);
    uiTitle(game, "Sector Siege", 90, 202, 90);
    for (int i = 0; i < 4; ++i) {
        Rectangle row = choiceBounds(i);
        uiButton(game, row, choices[i].label, i == game->menu->focus);
        if (i == game->menu->focus)
            DrawRectangle((int)(row.x + row.width - 34), (int)(row.y + 28), 12, 2, UI_INK);
    }
    if (game->menu->pending)
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(UI_INK, Clamp(game->menu->transition / 0.16f, 0, 1)));
}
