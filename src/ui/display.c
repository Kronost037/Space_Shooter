#include "display.h"
#include "widgets.h"
#include "../game_over.h"
#include "../menu.h"
#include "../name_entry.h"
#include "../panel.h"
#include "../settings.h"

const Rectangle resumeButton = { 527, 477, 210, 48 };
const Rectangle leaveButton = { 763, 477, 210, 48 };

static void drawPause(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.63f));
    uiSurface((Rectangle){490, 310, 520, 250});
    uiTitle(game, "Flight paused", 527, 328, 55);
    uiLabel(game, "Flight time is suspended.", 530, 414, 24, UI_MUTED);
    uiButton(game, resumeButton, "Resume", CheckCollisionPointRec(GetMousePosition(), resumeButton));
    uiButton(game, leaveButton, "Abandon run", CheckCollisionPointRec(GetMousePosition(), leaveButton));
}

Rectangle screenViewport(void) {
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();
    float scale = fmaxf(0.001f, fminf(width / SCREEN_WIDTH, height / SCREEN_HEIGHT));
    return (Rectangle){ (width - SCREEN_WIDTH * scale) * 0.5f,
                         (height - SCREEN_HEIGHT * scale) * 0.5f,
                         SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale };
}

void renderFrame(Game *game, RenderTexture2D frame, Rectangle viewport) {
    bool inFlight = game->currentState == STATE_GAME || game->currentState == STATE_PAUSE;
    if (inFlight) {
        BeginTextureMode(frame);
        ClearBackground(BLACK);
        drawGame(game);
        EndTextureMode();
    }
    ClearBackground(BLACK);
    if (inFlight)
        DrawTexturePro(frame.texture, (Rectangle){0, 0, SCREEN_WIDTH, -SCREEN_HEIGHT},
                       viewport, (Vector2){0}, 0, WHITE);
    BeginScissorMode((int)roundf(viewport.x), (int)roundf(viewport.y),
                     (int)roundf(viewport.width), (int)roundf(viewport.height));
    BeginMode2D((Camera2D){.offset = {viewport.x, viewport.y}, .zoom = viewport.width / SCREEN_WIDTH});
    if (inFlight) {
        Camera2D camera = game->camera;
        camera.target = Vector2Lerp(game->previousCameraTarget, camera.target, Clamp(game->accumulator * 120, 0, 1));
        drawPanel(game, camera);
        if (game->currentState == STATE_PAUSE) drawPause(game);
    } else {
        switch (game->currentState) {
        case STATE_MENU: drawMenu(game); break;
        case STATE_NAME_ENTRY: drawNameEntry(game); break;
        case STATE_GAME_OVER: drawGameOver(game); break;
        case STATE_LEADERBOARD: drawLeaderboard(game); break;
        case STATE_SETTING: drawSettings(game); break;
        default: break;
        }
    }
    if (game->screenAge < 0.22f && game->currentState != STATE_PAUSE) {
        float veil = 1.0f - game->screenAge / 0.22f;
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(UI_INK, veil * veil * 0.85f));
    }
    EndMode2D();
    EndScissorMode();
}
