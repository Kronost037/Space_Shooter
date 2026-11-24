#include "cinematic.h"
#include "widgets.h"
#include "../flight/flight.h"
#include "../flight/mission.h"
#include "../arena.h"

static const float sceneDuration = 2.8f;

void playScene(Game *game, FlightScene scene) {
    game->scene = scene;
    game->sceneTime = 0;
}

bool updateScene(Game *game, float dt, bool skip) {
    if (game->scene == SCENE_NONE) return false;
    game->sceneTime += dt;
    if (skip || game->sceneTime >= sceneDuration) game->scene = SCENE_NONE;
    // Consume the last cinematic frame so a skip cannot also fire or pause the game.
    return true;
}

static void drawGate(Vector2 center, float time, float radius, float glow) {
    DrawCircleGradient(center, radius * 1.4f, Fade(UI_ICE, glow * 0.34f), Fade(UI_ICE, 0));
    for (int i = 0; i < 4; ++i) {
        float angle = time * 19 + i * 90;
        DrawRing(center, radius - 3, radius + 3, angle, angle + 66, 29, Fade(UI_ICE, glow));
    }
}

void drawScene(const Game *game) {
    float t = game->sceneTime;
    float p = Clamp(t / sceneDuration, 0, 1);
    float ease = p * p * (3 - 2 * p);
    const char *line = "";
    Camera2D drift = {.target = {1620 + 640 * ease, 1750 + 120 * ease}};
    drawArenaBackdrop(drift, game->sky);
    switch (game->scene) {
    case SCENE_LAUNCH: {
        Vector2 scout = {290 + 1030 * ease, 580 - 290 * ease};
        drawGate((Vector2){1090,354}, t, 106, 0.58f);
        drawShip(game->pilotTexture, scout, Vector2Normalize((Vector2){1,-0.27f}),
                 155 - 67 * ease, WHITE, CYAN);
        line = "Aster Lead: find us a way through.";
        break;
    }
    case SCENE_ESCORT: {
        Vector2 craft = {480 + 470 * ease, 540 - 94 * ease};
        drawGate((Vector2){1040,390}, t, 140, 0.42f);
        drawShip(game->pilotTexture, (Vector2){craft.x - 122, craft.y + 91}, (Vector2){1,0}, 105, WHITE, CYAN);
        drawTransport(craft, Vector2Normalize((Vector2){1,-0.2f}), 2.6f);
        line = "Mercy Six: ready for pickup.";
        break;
    }
    case SCENE_CARRIER:
        drawGate((Vector2){790,440}, t, 250 - 56 * ease, 0.54f);
        drawShip(game->carrierTexture, (Vector2){790,520 - 96 * ease},
                 direction(-PI / 2 + 0.15f * (1 - ease)), 230 + 122 * ease,
                 Fade(WHITE, 0.45f + 0.55f * ease), AMBER);
        line = "Aster Lead: Carrier at the jump gate.";
        break;
    case SCENE_CLEAR:
        drawGate((Vector2){980,412}, t, 130 + 30 * ease, 0.9f);
        for (int i = 0; i < 3; ++i) {
            float progress = Clamp((p - i * 0.15f) / 0.67f, 0, 1);
            Vector2 ship = Vector2Lerp((Vector2){230 + i * 60,590 - i * 64},
                                      (Vector2){980,412}, progress);
            if (progress < 0.97f)
                drawTransport(ship, Vector2Normalize((Vector2){1,-0.17f}), 1.65f - 0.85f * progress);
        }
        line = "Aster Rearguard: last ship through.";
        break;
    case SCENE_NONE:
        return;
    }
    DrawRectangle(0, 0, SCREEN_WIDTH, 106, UI_INK);
    DrawRectangle(0, SCREEN_HEIGHT - 122, SCREEN_WIDTH, 122, UI_INK);
    uiLabel(game, line, 92, SCREEN_HEIGHT - 84, 29, UI_PAPER);
    uiLabel(game, "Enter to skip", SCREEN_WIDTH - 278, SCREEN_HEIGHT - 79, 21, UI_MUTED);
    float fade = fmaxf(1 - t * 4, (p - 0.91f) / 0.09f);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, Clamp(fade, 0, 1)));
}
