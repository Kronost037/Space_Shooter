#include "panel.h"

#define CYAN ((Color){96, 222, 255, 255})
#define MUTED ((Color){161, 181, 215, 255})
#define WARDEN_ACCENT ((Color){207, 144, 255, 255})
#define AMBER ((Color){255, 194, 98, 255})

static void card(Rectangle rect, Color accent) {
    DrawRectangleRounded(rect, 0.12f, 8, (Color){8, 13, 28, 230});
    DrawRectangleRoundedLines(rect, 0.12f, 8, Fade(accent, 0.36f));
}

static void drawHudText(const Game *game, const char *label, float x, float y, float size, Color color) {
    DrawTextEx(game->bodyFont, label, (Vector2){x, y}, size, 0.8f, color);
}

static void centered(const Game *game, const char *label, float y, float size, Color color) {
    float width = MeasureTextEx(game->bodyFont, label, size, 0.8f).x;
    drawHudText(game, label, (SCREEN_WIDTH - width) * 0.5f, y, size, color);
}

static void bar(Rectangle rect, float value, Color accent) {
    DrawRectangleRec(rect, (Color){41, 49, 76, 255});
    rect.width *= Clamp(value, 0.0f, 1.0f);
    DrawRectangleRec(rect, accent);
}

static Vector2 mapPoint(Rectangle area, Vector2 position) {
    return (Vector2){
        area.x + position.x / WORLD_WIDTH * area.width,
        area.y + position.y / WORLD_HEIGHT * area.height
    };
}

static void drawRadar(const Game *game, Camera2D camera) {
    Rectangle panel = { SCREEN_WIDTH - 225, 22, 203, 181 };
    Rectangle grid = { panel.x + 13, panel.y + 36, panel.width - 26, panel.height - 51 };
    card(panel, CYAN);
    drawHudText(game, "SECTOR RADAR", (int)panel.x + 13, (int)panel.y + 10, 18, MUTED);
    DrawRectangleLinesEx(grid, 1.0f, Fade(MUTED, 0.40f));

    for (int x = 1; x < WORLD_COLS; ++x) {
        float line = grid.x + grid.width * (float)x / WORLD_COLS;
        DrawLineV((Vector2){line, grid.y}, (Vector2){line, grid.y + grid.height}, Fade(MUTED, 0.22f));
    }
    for (int y = 1; y < WORLD_ROWS; ++y) {
        float line = grid.y + grid.height * (float)y / WORLD_ROWS;
        DrawLineV((Vector2){grid.x, line}, (Vector2){grid.x + grid.width, line}, Fade(MUTED, 0.22f));
    }

    Vector2 topLeft = mapPoint(grid, GetScreenToWorld2D((Vector2){0, 0}, camera));
    Vector2 bottomRight = mapPoint(grid, GetScreenToWorld2D((Vector2){SCREEN_WIDTH, SCREEN_HEIGHT}, camera));
    topLeft = Vector2Clamp(topLeft, (Vector2){grid.x, grid.y}, (Vector2){grid.x + grid.width, grid.y + grid.height});
    bottomRight = Vector2Clamp(bottomRight, (Vector2){grid.x, grid.y}, (Vector2){grid.x + grid.width, grid.y + grid.height});
    DrawRectangleLinesEx((Rectangle){topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y},
                         1.0f, Fade(CYAN, 0.48f));

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        const Ship *scout = &game->scouts[i];
        if (!scout->lives || Vector2Distance(scout->position, game->player.position) > 1050.0f) continue;
        DrawCircleV(mapPoint(grid, scout->position), 2.0f, (Color){255, 94, 113, 255});
    }
    for (int i = 0; i < MAX_ELITES; ++i) {
        const Elite *elite = &game->elites[i];
        if (!elite->unlocked || !elite->ship.lives) continue;
        DrawPoly(mapPoint(grid, elite->ship.position), 4, 5.0f, 0.0f,
                 i == MAX_ELITES - 1 ? AMBER : WARDEN_ACCENT);
    }
    DrawCircleV(mapPoint(grid, game->player.position), 4.0f, CYAN);
}

static int nearestTarget(const Game *game) {
    int best = -1;
    float bestDistance = INFINITY;
    for (int i = 0; i < MAX_ELITES; ++i) {
        const Elite *elite = &game->elites[i];
        if (!elite->unlocked || elite->ship.lives <= 0) continue;
        float distance = Vector2Distance(game->player.position, elite->ship.position);
        if (distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

static void drawObjective(const Game *game, Camera2D camera) {
    int index = nearestTarget(game);
    if (index < 0) return;
    const Elite *elite = &game->elites[index];
    bool carrier = index == MAX_ELITES - 1;
    Color accent = carrier ? AMBER : WARDEN_ACCENT;
    Vector2 screen = GetWorldToScreen2D(elite->ship.position, camera);
    bool ahead = screen.x > 86 && screen.x < SCREEN_WIDTH - 86 &&
                 screen.y > 92 && screen.y < SCREEN_HEIGHT - 92;
    if (ahead) {
        centered(game, carrier ? "COMMAND CARRIER" : "SECTOR WARDEN", 21, 24, accent);
        bar((Rectangle){SCREEN_WIDTH * 0.5f - 155.0f, 51, 310, 6},
            (float)elite->ship.lives / (float)elite->maxLives, accent);
        if (elite->phase == ELITE_WARNING) {
            const char *attack = elite->attack == ATTACK_LANES ? "LANE BATTERY" : elite->attack == ATTACK_LANCE ? (elite->time > 0.48f ? "TRACKING" : "LANCE LOCKED") :
                                 elite->attack == ATTACK_RING ? "RING BURST" : "FAN VOLLEY";
            centered(game, attack, 61, 19, AMBER);
        } else if (elite->phase == ELITE_RECOVERY) {
            centered(game, "CORE EXPOSED   /   DOUBLE DAMAGE", 61, 19, CYAN);
        } else if (carrier) {
            centered(game, "ARMORED", 61, 19, MUTED);
        }
    } else {
        Vector2 center = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f };
        Vector2 delta = Vector2Subtract(screen, center);
        const float halfWidth = SCREEN_WIDTH * 0.5f - 96.0f;
        const float halfHeight = SCREEN_HEIGHT * 0.5f - 102.0f;
        float edge = 1.0f / fmaxf(fabsf(delta.x) / halfWidth, fabsf(delta.y) / halfHeight);
        Vector2 marker = Vector2Add(center, Vector2Scale(delta, edge));
        // Reserve space around the top corners for status and radar.
        if (marker.y < 222.0f && (marker.x < 270.0f || marker.x > SCREEN_WIDTH - 265.0f)) marker.y = 222.0f;
        Vector2 arrow = Vector2Normalize(delta);
        Vector2 wing = { -arrow.y, arrow.x };
        DrawTriangle(Vector2Add(marker, Vector2Scale(arrow, 11.0f)),
                     Vector2Subtract(Vector2Subtract(marker, Vector2Scale(arrow, 7.0f)), Vector2Scale(wing, 7.0f)),
                     Vector2Add(Vector2Subtract(marker, Vector2Scale(arrow, 7.0f)), Vector2Scale(wing, 7.0f)),
                     accent);
        const char *label = carrier ? "CARRIER" : "WARDEN";
        drawHudText(game, label, marker.x - MeasureTextEx(game->bodyFont, label, 17.0f, 0.8f).x * 0.5f, marker.y + 13.0f, 17.0f, accent);
    }
}

void drawPanel(const Game *game, Camera2D camera) {
    card((Rectangle){22, 22, 228, 120}, CYAN);
    drawHudText(game, "HULL", 36, 33, 15, MUTED);
    for (int i = 0; i < MAX_LIVES; ++i) {
        DrawRectangleRounded((Rectangle){37.0f + (float)i * 24.0f, 56, 17, 7}, 0.7f, 6,
                             i < game->player.lives ? CYAN : (Color){42, 52, 77, 255});
    }
    drawHudText(game, "SCORE", 133, 33, 15, MUTED);
    drawHudText(game, TextFormat("%05d", game->score), 133, 48, 32, RAYWHITE);
    drawHudText(game, TextFormat("SECTOR THREAT %d", 1 + game->wardensCleared), 36, 88, 15, MUTED);
    drawHudText(game, TextFormat("SCOUT REWARDS  %d / %d", game->scoutRewards, SCOUT_REWARDS_PER_STAGE),
             36, 110, 17, MUTED);
    if (game->scoreToast > 0.0f) {
        drawHudText(game, TextFormat("+%d", game->lastAward), GetWorldToScreen2D(game->payoutPosition, camera).x, GetWorldToScreen2D(game->payoutPosition, camera).y - 35, 20, Fade(CYAN, game->scoreToast / 0.85f));
    }

    if (game->showRadar) drawRadar(game, camera);
    drawObjective(game, camera);

    bool ready = game->pulseCharge >= PULSE_RECHARGE;
    Rectangle pulse = {22, 150, 228, 54};
    card(pulse, CYAN);
    drawHudText(game, ready ? "SPACE  /  PULSE READY" :
             TextFormat("PULSE  /  %.1fs", PULSE_RECHARGE - game->pulseCharge),
             36, 161, 16, ready ? CYAN : MUTED);
    bar((Rectangle){36, 187, 200, 4}, game->pulseCharge / PULSE_RECHARGE, CYAN);

    const char *objective = game->wardensCleared < MAX_ELITES - 1 ?
        TextFormat("WARDENS  %d / %d", game->wardensCleared, MAX_ELITES - 1) : "DESTROY THE CARRIER";
    centered(game, objective, SCREEN_HEIGHT - 44, 21, game->wardensCleared < MAX_ELITES - 1 ? WARDEN_ACCENT : AMBER);
    if (game->settings.timerEnabled) {
        int remaining = (int)ceilf(game->settings.timerMinutes * 60 - game->elapsedTime);
        const char *clock = TextFormat("%d:%02d", remaining / 60, remaining % 60);
        drawHudText(game, clock, 36, 215, 26, remaining <= 15 ? AMBER : RAYWHITE);
    }
    if (game->elapsedTime < 12.0f) {
        centered(game, "ARROWS MOVE   /   WASD FIRE   /   M RADAR", SCREEN_HEIGHT - 71, 18,
                 Fade(MUTED, Clamp((12.0f - game->elapsedTime) / 2.0f, 0.0f, 1.0f)));
    }
}
