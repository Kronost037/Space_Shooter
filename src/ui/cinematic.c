#include "cinematic.h"
#include "widgets.h"
#include "../flight/flight.h"
#include "../flight/mission.h"
#include "../arena.h"

static const float sceneDuration = 7.0f;

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

static void drawGate(const Game *game, Vector2 center, float height, float light) {
    float breathe = 0.5f + 0.5f * sinf(game->sceneTime * 2.2f);
    DrawCircleGradient(center, height * 0.46f, Fade(UI_ICE, light * (0.12f + 0.06f * breathe)), Fade(UI_ICE, 0));
    drawShip(game->gateTexture, center, (Vector2){0,-1}, height, WHITE, UI_MUTED);
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
        Vector2 scout = {180 + 800 * ease, 610 - 240 * ease - 65 * sinf(p * PI)};
        drawGate(game, (Vector2){1040 - 58 * ease,365}, 350 - 45 * ease, 0.7f);
        drawShip(game->pilotTexture, scout, direction(-0.42f + 0.38f * ease),
                 265 - 222 * ease, WHITE, CYAN);
        line = "Mercy Six: We have your lights, Lead. Take us through.";
        break;
    }
    case SCENE_ESCORT: {
        Vector2 craft = {460 + 440 * p, 530 - 102 * p - 14 * sinf(PI * p)};
        drawGate(game, (Vector2){1150 - 45 * p,345}, 320, 0.4f);
        drawShip(game->pilotTexture, (Vector2){craft.x - 202 + 18 * ease, craft.y + 128 - 16 * ease}, direction(-0.26f + 0.1f * ease), 91, WHITE, CYAN);
        drawTransport(game, craft, direction(-0.26f + 0.08f * ease), 300 - 45 * ease, 0.7f);
        line = "Mercy Six: ready for pickup.";
        break;
    }
    case SCENE_CARRIER:
        drawGate(game, (Vector2){850 - 75 * ease,435}, 555 - 40 * ease, 0.8f);
        drawShip(game->carrierTexture, (Vector2){790,520 - 96 * ease},
                 direction(-PI / 2 + 0.15f * (1 - ease)), 170 + 220 * ease,
                 ColorLerp((Color){78,92,108,255}, WHITE, ease), AMBER);
        line = "Aster Lead: Carrier at the jump gate.";
        break;
    case SCENE_CLEAR:
        drawGate(game, (Vector2){980,412}, 392 + 18 * ease, 1);
        for (int i = 0; i < 3; ++i) {
            float progress = Clamp((p - i * 0.15f) / 0.67f, 0, 1);
            Vector2 ship = Vector2Lerp((Vector2){230 + i * 60,590 - i * 64},
                                      (Vector2){980,412}, progress);
            float jump = Clamp((progress - 0.75f) * 4, 0, 1);
            float length = (158 - 83 * progress) * (1 - jump);
            if (length >= 1) drawTransport(game, ship, direction(-0.17f), length, 0.6f + jump);
            if (jump > 0 && jump < 1)
                DrawCircleGradient(ship, 53 * jump, Fade(UI_ICE, sinf(PI * jump) * 0.65f), Fade(UI_ICE, 0));
        }
        line = "Aster Rearguard: last ship through.";
        break;
    case SCENE_NONE:
        return;
    }
    DrawRectangle(0, 0, SCREEN_WIDTH, 106, UI_INK);
    DrawRectangle(0, SCREEN_HEIGHT - 122, SCREEN_WIDTH, 122, UI_INK);
    bool reply = t >= 3.4f;
    bool captain = reply;
    switch (game->scene) {
    case SCENE_LAUNCH:
        line = reply ? "Our ward is running on reserve. We'll follow your lights."
                     : "Mercy Six, your corridor is dark. Sending a pilot ahead.";
        break;
    case SCENE_ESCORT:
        line = reply ? "That's the medicine. Hold them off until the crane clears."
                     : "Berth three is yours. The last cold crate is on the hook.";
        break;
    case SCENE_CARRIER:
        line = reply ? "We're under the gate. You have a clear line past our bow."
                     : "Carrier batteries are turning. Mercy Six, hold at the pier.";
        break;
    case SCENE_CLEAR:
        line = reply ? "All beds accounted for. Lead, we have the other side."
                     : "Mercy Six, your signal dropped. Do you read?";
        break;
    default: break;
    }
    if (IsTextureValid(game->crewPortraits)) {
        float w = game->crewPortraits.width * 0.5f;
        DrawTexturePro(game->crewPortraits,(Rectangle){captain ? 0 : w,0,w,(float)game->crewPortraits.height},
                       (Rectangle){74,SCREEN_HEIGHT-145,136,136},(Vector2){0},0,WHITE);
    }
    uiLabel(game,captain ? "CAPT. SEN  /  MERCY SIX" : "VALE  /  DOCK CONTROL",232,SCREEN_HEIGHT-104,21,UI_ICE);
    uiLabel(game,line,232,SCREEN_HEIGHT-66,26,UI_PAPER);
    float fade = fmaxf(1 - t * 4, (p - 0.91f) / 0.09f);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, Clamp(fade, 0, 1)));
}
