#include "widgets.h"
#include "../arena.h"

void uiBackdrop(const Game *game, float time) {
    if (IsTextureValid(game->drydock)) {
        Texture2D plate = game->drydock;
        float zoom = 1.04f;
        float fit = fmaxf((float)SCREEN_WIDTH / plate.width, (float)SCREEN_HEIGHT / plate.height) * zoom;
        float w = plate.width * fit, h = plate.height * fit;
        float x = (SCREEN_WIDTH - w) * (0.5f + 0.25f * sinf(time * 0.10f));
        float y = (SCREEN_HEIGHT - h) * (0.5f + 0.18f * cosf(time * 0.12f));
        DrawTexturePro(plate, (Rectangle){0,0,(float)plate.width,(float)plate.height},
                       (Rectangle){x,y,w,h}, (Vector2){0}, 0, WHITE);
    } else {
        drawArenaBackdrop((Camera2D){.target={2100,1600}}, game->sky);
    }
    DrawRectangleGradientH(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){4,10,17,170}, (Color){5,12,21,35});
}

void uiSurface(Rectangle area) {
    DrawRectangleRec(area, (Color){12,22,34,245});
    DrawRectangleLinesEx(area, 1, (Color){72,91,106,255});
    DrawRectangle((int)area.x, (int)area.y, 60, 3, UI_AMBER);
}

bool uiCompact(void) {
    return fminf((float)GetScreenWidth() / SCREEN_WIDTH, (float)GetScreenHeight() / SCREEN_HEIGHT) < 0.62f;
}

float uiTextSize(float size) {
    float scale = fminf((float)GetScreenWidth() / SCREEN_WIDTH, (float)GetScreenHeight() / SCREEN_HEIGHT);
    return fmaxf(size, 15.0f / fmaxf(scale, 0.4f));
}

float uiTextWidth(const Game *game, const char *text, float size) {
    return MeasureTextEx(game->bodyFont, text, uiTextSize(size), 0).x;
}

void uiLabel(const Game *game, const char *text, float x, float y, float size, Color color) {
    DrawTextEx(game->bodyFont, text, (Vector2){roundf(x),roundf(y)}, uiTextSize(size), 0, color);
}

void uiTitle(const Game *game, const char *text, float x, float y, float size) {
    DrawTextEx(game->bodyFont, text, (Vector2){roundf(x),roundf(y)}, size, 0, UI_PAPER);
}

void uiButton(const Game *game, Rectangle bounds, const char *label, bool selected) {
    DrawRectangleRec(bounds, selected ? UI_AMBER : (Color){22,37,51,255});
    DrawRectangleLinesEx(bounds, 1, selected ? UI_AMBER : (Color){70,90,103,255});
    uiLabel(game, label, bounds.x + 19, bounds.y + (bounds.height - uiTextSize(25)) / 2, 25,
            selected ? UI_INK : UI_PAPER);
}

bool uiClicked(Rectangle bounds) {
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), bounds);
}


void uiAction(const Game *game, Rectangle bounds, const char *label, bool active) {
    Color ink = active ? UI_AMBER : UI_PAPER;
    if (active) {
        DrawRectangleGradientH((int)bounds.x, (int)bounds.y, (int)bounds.width, (int)bounds.height,
                               (Color){194,133,63,54}, (Color){194,133,63,0});
        DrawRectangleRec((Rectangle){bounds.x, bounds.y + 9, 4, bounds.height - 18}, UI_AMBER);
    }
    float size = uiTextSize(30);
    uiLabel(game, label, bounds.x + 20, bounds.y + (bounds.height - size) / 2, size, ink);
}
