#include "panel.h"

#include <stdio.h>
#include <math.h>

#define RADAR_DETECTION_TIME 5.0f

static void drawGlowText(Font font, const char *text, Vector2 pos, float size, float spacing, Color textColor, Color glowColor) {
    for (int i = 3; i >= 1; i--) {
        Vector2 p = { pos.x + (float)i, pos.y + (float)i };
        Color c = glowColor;
        c.a = (unsigned char)(glowColor.a / (i + 1));
        DrawTextEx(font, text, p, size, spacing, c);
    }
    DrawTextEx(font, text, pos, size, spacing, textColor);
}

static void drawSoftPanel(Rectangle r, Color fill, Color border) {
    DrawRectangleRec(r, fill);
    DrawRectangleLinesEx(r, 1.5f, border);
    DrawRectangleRec((Rectangle){ r.x + 1.0f, r.y + 1.0f, r.width - 2.0f, 1.0f }, (Color){ 255, 255, 255, 18 });
}

static void drawSeparator(float x, float y, float w) {
    DrawRectangleRec((Rectangle){ x, y, w, 1.0f }, (Color){ 70, 145, 220, 110 });
    DrawRectangleRec((Rectangle){ x, y + 3.0f, w * 0.78f, 1.0f }, (Color){ 130, 70, 200, 70 });
}

static void drawSmallLabel(Font font, const char *text, Vector2 pos) {
    DrawTextEx(font, text, pos, 16.0f, 1.0f, (Color){ 180, 190, 205, 255 });
}

static void drawMetricCard(Game *game, Rectangle r, const char *label, const char *value, Color accent) {
    drawSoftPanel(r, (Color){ 14, 18, 27, 255 }, (Color){ 90, 110, 135, 80 });
    DrawRectangleRec((Rectangle){ r.x + 1.0f, r.y + 1.0f, r.width - 2.0f, 3.0f }, accent);
    DrawRectangleRec((Rectangle){ r.x + 1.0f, r.y + 4.0f, r.width - 2.0f, 1.0f }, (Color){ 255, 255, 255, 18 });

    drawSmallLabel(game->font, label, (Vector2){ r.x + 14.0f, r.y + 12.0f });
    drawGlowText(game->font, value, (Vector2){ r.x + 14.0f, r.y + 34.0f }, 24.0f, 1.0f, (Color){ 235, 242, 248, 255 }, accent);
}

static void drawHealthBar(Game *game, Rectangle r) {
    float ratio = 0.0f;
    if (MAX_LIVES > 0) {
        ratio = (float)game->player.lives / (float)MAX_LIVES;
    }
    ratio = fmaxf(0.0f, fminf(1.0f, ratio));

    drawSoftPanel(r, (Color){ 13, 16, 24, 255 }, (Color){ 90, 110, 135, 80 });

    DrawRectangleRec((Rectangle){ r.x + 2.0f, r.y + 2.0f, (r.width - 4.0f) * ratio, r.height - 4.0f }, (Color){ 40, 180, 255, 220 });
    DrawRectangleRec((Rectangle){ r.x + 2.0f, r.y + 2.0f, (r.width - 4.0f) * ratio, 3.0f }, (Color){ 230, 250, 255, 70 });

    for (int i = 1; i < MAX_LIVES; i++) {
        float x = r.x + 2.0f + ((r.width - 4.0f) / (float)MAX_LIVES) * (float)i;
        DrawRectangleRec((Rectangle){ x - 1.0f, r.y + 2.0f, 2.0f, r.height - 4.0f }, (Color){ 75, 92, 110, 120 });
    }

    DrawRectangleLinesEx(r, 1.5f, (Color){ 100, 125, 150, 90 });
}

static void drawMiniMap(Game *game, Rectangle area) {
    drawSoftPanel(area, (Color){ 12, 14, 20, 255 }, (Color){ 100, 140, 175, 90 });

    Rectangle inner = {
        area.x + 8.0f,
        area.y + 8.0f,
        area.width - 16.0f,
        area.height - 16.0f
    };

    DrawRectangleRec(inner, (Color){ 8, 10, 15, 255 });
    DrawRectangleLinesEx(inner, 1.0f, (Color){ 75, 95, 115, 80 });

    int cols = game->world.cols;
    int rows = game->world.rows;

    float cellW = inner.width / (float)cols;
    float cellH = inner.height / (float)rows;

    int playerSX = (int)(game->player.entity_pos.x / (float)game->world.sectorWidth);
    int playerSY = (int)(game->player.entity_pos.y / (float)game->world.sectorHeight);

    if (playerSX < 0) playerSX = 0;
    if (playerSY < 0) playerSY = 0;
    if (playerSX >= cols) playerSX = cols - 1;
    if (playerSY >= rows) playerSY = rows - 1;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Rectangle cell = {
                inner.x + (float)x * cellW,
                inner.y + (float)y * cellH,
                cellW,
                cellH
            };

            bool playerCell = (x == playerSX && y == playerSY);

            DrawRectangleRec(cell, playerCell ? (Color){ 18, 56, 82, 150 } : (Color){ 11, 14, 20, 255 });
            DrawRectangleLinesEx(cell, 1.0f, playerCell ? (Color){ 65, 210, 255, 180 } : (Color){ 55, 75, 95, 65 });
        }
    }


    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game->enemies[i].lives <= 0) continue;

        float age = game->elapsedTime - game->enemies[i].last_seen;

        if(age > RADAR_DETECTION_TIME) continue; 
        
        float px = inner.x + (game->enemies[i].entity_pos.x / (float)game->world.width) * inner.width;
        float py = inner.y + (game->enemies[i].entity_pos.y / (float)game->world.height) * inner.height;

        DrawCircleV((Vector2){ px, py }, 2.3f, (Color){ 255, 95, 120, 255 });
    }

    float playerX = inner.x + ((game->player.entity_pos.x + 0.5f) / (float)game->world.width) * inner.width;
    float playerY = inner.y + ((game->player.entity_pos.y + 0.5f) / (float)game->world.height) * inner.height;

    DrawCircleV((Vector2){ playerX, playerY }, 4.2f, (Color){ 45, 220, 255, 255 });
    DrawCircleLines((int)playerX, (int)playerY, 7.5f, (Color){ 140, 240, 255, 160 });
}

void drawPanel(Game *game) {
    if (!game->showPanel) return;

    Rectangle panel = {
        (float)game->gameWidth,
        0.0f,
        (float)PANEL_WIDTH,
        (float)game->gameHeight
    };

    DrawRectangleRec(panel, (Color){ 7, 9, 14, 255 });
    DrawRectangleLinesEx(panel, 2.0f, (Color){ 85, 105, 130, 90 });

    drawGlowText(
        game->font,
        "SPACE SHOOTER",
        (Vector2){ panel.x + 35.0f, 18.0f },
        48.0f,
        1.4f,
        (Color){ 235, 242, 248, 255 },
        (Color){ 80, 170, 255, 220 }
    );

    drawSeparator(panel.x + 16.0f, 64.0f, panel.width - 32.0f);

    Rectangle nameCard = { panel.x + 16.0f, 82.0f, panel.width - 32.0f, 65.0f };
    char nameBuf[64];
    snprintf(nameBuf, sizeof(nameBuf), "%s", game->playerName[0] ? game->playerName : "PLAYER");
    drawMetricCard(game, nameCard, "OPERATIVE", nameBuf, (Color){ 0, 210, 255, 220 });

    Rectangle scoreCard = { panel.x + 16.0f, 155.0f, panel.width - 32.0f, 65.0f };
    char scoreBuf[32];
    snprintf(scoreBuf, sizeof(scoreBuf), "%d", game->enemiesKilled);
    drawMetricCard(game, scoreCard, "SCORE", scoreBuf, (Color){ 120, 90, 255, 220 });

    Rectangle healthLabelRect = { panel.x + 16.0f, 230.0f, panel.width - 32.0f, 24.0f };
    drawSmallLabel(game->font, "HEALTH", (Vector2){ healthLabelRect.x, healthLabelRect.y });

    Rectangle healthBar = { panel.x + 16.0f, 248.0f, panel.width - 32.0f, 22.0f };
    drawHealthBar(game, healthBar);

    drawSmallLabel(game->font, "TACTICAL MAP", (Vector2){ panel.x + 16.0f, 288.0f });

    Rectangle minimap = { panel.x + 16.0f, 314.0f, panel.width - 32.0f, 222.0f };
    drawMiniMap(game, minimap);

    drawSeparator(panel.x + 16.0f, 552.0f, panel.width - 32.0f);

    Rectangle hintCard = { panel.x + 16.0f, 570.0f, panel.width - 32.0f, 114.0f };
    drawSoftPanel(hintCard, (Color){ 12, 15, 22, 255 }, (Color){ 80, 100, 125, 70 });

    DrawTextEx(game->font, "M to TOGGLE PANEL", (Vector2){ hintCard.x + 14.0f, hintCard.y + 14.0f }, 17.0f, 1.0f, (Color){ 180, 190, 205, 255 });
    DrawTextEx(game->font, "WASD to SHOOT",     (Vector2){ hintCard.x + 14.0f, hintCard.y + 40.0f }, 17.0f, 1.0f, (Color){ 180, 190, 205, 255 });
    DrawTextEx(game->font, "ARROW Keys to MOVE",    (Vector2){ hintCard.x + 14.0f, hintCard.y + 66.0f }, 17.0f, 1.0f, (Color){ 180, 190, 205, 255 });
}
