#include "widgets.h"
#include "../arena.h"

void uiBackdrop(const Game *game, float time) {
    Camera2D camera = {.target = {2100 + sinf(time * 0.07f) * 180, 1600 + cosf(time * 0.05f) * 120}};
    drawArenaBackdrop(camera, game->sky);
    DrawRectangleGradientH(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){5,13,25,238}, (Color){9,17,31,45});
}

void uiSurface(Rectangle area) {
    DrawRectangleRec(area, (Color){12,22,34,245});
    DrawRectangleLinesEx(area, 1, (Color){72,91,106,255});
    DrawRectangle((int)area.x, (int)area.y, 60, 3, UI_AMBER);
}

void uiLabel(const Game *game, const char *text, float x, float y, float size, Color color) {
    DrawTextEx(game->bodyFont, text, (Vector2){roundf(x),roundf(y)}, size, 0, color);
}

void uiTitle(const Game *game, const char *text, float x, float y, float size) {
    DrawTextEx(game->font, text, (Vector2){roundf(x),roundf(y)}, size, 1, UI_PAPER);
}

void uiButton(const Game *game, Rectangle bounds, const char *label, bool selected) {
    DrawRectangleRec(bounds, selected ? UI_AMBER : (Color){22,37,51,255});
    DrawRectangleLinesEx(bounds, 1, selected ? UI_AMBER : (Color){70,90,103,255});
    uiLabel(game, label, bounds.x + 19, bounds.y + (bounds.height - 25) / 2, 25,
            selected ? UI_INK : UI_PAPER);
}

bool uiClicked(Rectangle bounds) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), bounds);
}
