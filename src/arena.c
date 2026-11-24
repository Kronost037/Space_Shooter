#include "arena.h"

#define AMBER ((Color){255,194,98,255})

static unsigned starHash(unsigned seed) {
    seed ^= seed >> 16;
    seed *= 0x7feb352du;
    seed ^= seed >> 15;
    seed *= 0x846ca68bu;
    return seed ^ (seed >> 16);
}

static void drawStarLayer(Vector2 target, float parallax, int spacing, unsigned salt) {
    Vector2 scroll = Vector2Scale(target, parallax);
    int firstX = (int)floorf(scroll.x / (float)spacing);
    int firstY = (int)floorf(scroll.y / (float)spacing);
    for (int y = firstY; y <= firstY + SCREEN_HEIGHT / spacing + 1; ++y) {
        for (int x = firstX; x <= firstX + SCREEN_WIDTH / spacing + 1; ++x) {
            unsigned hash = starHash((unsigned)x * 1597334677u ^ (unsigned)y * 3812015801u ^ salt);
            Vector2 p = { (float)(x * spacing) + (float)(hash % (unsigned)spacing) - scroll.x,
                          (float)(y * spacing) + (float)((hash >> 8) % (unsigned)spacing) - scroll.y };
            Color color = (hash & 1u) ? (Color){128, 168, 244, 140} : (Color){150, 98, 224, 110};
            DrawCircleV(p, 0.8f + (float)((hash >> 5) % 4u) * 0.31f, color);
        }
    }
}

void drawArenaBackdrop(Camera2D camera, Texture2D sky) {
    if (IsTextureValid(sky)) {
        Rectangle destination = {-camera.target.x * 0.026f, -camera.target.y * 0.025f, 1680, 1120};
        DrawTexturePro(sky, (Rectangle){0,0,(float)sky.width,(float)sky.height}, destination, (Vector2){0}, 0, WHITE);
    } else {
        ClearBackground((Color){8, 10, 25, 255});
    }
    drawStarLayer(camera.target, 0.13f, 104, 137u);
    drawStarLayer(camera.target, 0.33f, 178, 761u);
}

static void drawSectorGrid(void) {
    Color grid = { 108, 142, 224, 36 };
    Color edge = { 126, 188, 255, 170 };
    for (int x = 1; x < WORLD_COLS; ++x) {
        float boundary = (float)(x * WORLD_SECTOR_WIDTH);
        DrawLineEx((Vector2){boundary, 0}, (Vector2){boundary, WORLD_HEIGHT}, 1.0f, grid);
    }
    for (int y = 1; y < WORLD_ROWS; ++y) {
        float boundary = (float)(y * WORLD_SECTOR_HEIGHT);
        DrawLineEx((Vector2){0, boundary}, (Vector2){WORLD_WIDTH, boundary}, 1.0f, grid);
    }
    DrawRectangleLinesEx((Rectangle){0, 0, WORLD_WIDTH, WORLD_HEIGHT}, 2.0f, edge);
}

void drawSectorLandmarks(const Game *game, Camera2D camera) {
    drawSectorGrid();
    for (int site = 0; site < MAX_ELITES; ++site) {
        Vector2 center = game->elites[site].patrolCenter;
        Vector2 screen = GetWorldToScreen2D(center, camera);
        if (screen.x < -350 || screen.x > SCREEN_WIDTH + 350 ||
            screen.y < -350 || screen.y > SCREEN_HEIGHT + 350) continue;
        bool released = game->elites[site].ship.lives == 0;
        Color steel = released ? (Color){67,128,136,150} : (Color){48,70,94,170};
        if (released && site < MAX_ELITES - 1) {
            // Navigation lights on the jump lane lead civilian traffic past this relay.
            float sweep = fmodf((float)game->elapsedTime * 0.3f, 1.0f);
            for (int lamp = 0; lamp < 8; ++lamp) {
                float phase = fmodf(sweep + lamp / 8.0f, 1.0f);
                Vector2 p = {center.x + lamp * 44.0f - 152.0f, center.y + 105.0f - lamp * 28.0f};
                DrawCircleV(p, 3, Fade((Color){133,211,206,255}, 0.24f + 0.65f * phase));
            }
        }
        if (site == 0) {
            for (int arc = 0; arc < 5; ++arc)
                DrawRing(center, 208, 220, arc * 72.0f, arc * 72.0f + 54, 30, steel);
        } else if (site == 1) {
            Vector2 moon = Vector2Add(center, (Vector2){175,95});
            DrawCircleV(moon, 118, (Color){36,65,86,255});
            DrawCircleV(Vector2Add(moon,(Vector2){-5,3}), 115, (Color){12,21,33,255});
        } else {
            for (int rail = -1; rail <= 1; rail += 2) {
                float x = center.x + 172 * rail;
                DrawRectangleRec((Rectangle){x, center.y-165, 9, 330}, steel);
                for (int j = -3; j <= 3; ++j)
                    DrawLineEx((Vector2){x,center.y+j*44}, (Vector2){x+42*rail,center.y+j*44-16}, 5,
                               site == 3 && game->elites[3].unlocked ? Fade(AMBER,0.48f) : steel);
            }
        }
    }
}
