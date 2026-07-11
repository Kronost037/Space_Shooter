#include "panel.h"

void drawPanel(Game *game) {
    if (!game->showPanel) return;

    Rectangle panel = {
        (float)game->gameWidth,
        0.0f,
        (float)PANEL_WIDTH,
        (float)game->gameHeight
    };

    DrawRectangleRec(panel, (Color){ 12, 16, 30, 255 });
    DrawRectangleLinesEx(panel, 2.0f, (Color){ 255, 255, 255, 40 });
    
    DrawTextEx(game->font, "Space Shooter", (Vector2){ panel.x + 22.0f, 20.0f }, 30.0f, 2.0f, (Color){ 230, 235, 245, 255 });

    
    DrawRectangleRec((Rectangle){ panel.x + 16.0f, 60.0f, panel.width - 32.0f, 2.0f }, (Color){ 60, 200, 255, 110 });
    DrawRectangleRec((Rectangle){ panel.x + 16.0f, 67.0f, panel.width - 70.0f, 1.0f }, (Color){ 145, 30, 230, 90 });

    char buf[128];
    snprintf(buf, sizeof(buf), "Enemies Killed: %d", game->enemiesKilled);
    DrawTextEx(game->font, buf, (Vector2){ panel.x + 18.0f, 76.0f }, 25.0f, 1.5f, (Color){ 220, 220, 235, 255 });

    DrawTextEx(game->font, "Press M to toggle panel", (Vector2){ panel.x + 18.0f, panel.height - 152.0f }, 20.0f, 2.0f, (Color){ 180, 190, 210, 255 });
    DrawTextEx(game->font, "WASD to shoot", (Vector2){ panel.x + 18.0f, panel.height - 126.0f }, 20.0f, 2.0f, (Color){ 180, 190, 210, 255 });
    DrawTextEx(game->font, "Arrow keys to move", (Vector2){ panel.x + 18.0f, panel.height - 100.0f }, 20.0f, 2.0f, (Color){ 180, 190, 210, 255 });
}
