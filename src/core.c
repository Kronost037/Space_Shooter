#include "core.h"
#include "game.h"
#include "menu.h"
#include "leaderboard.h"
#include "ui/widgets.h"
#include "settings.h"
#include "panel.h"
#include "name_entry.h"
#include "game_over.h"

#include <string.h>

static const Rectangle resumeButton = { 527, 477, 210, 48 };
static const Rectangle leaveButton = { 763, 477, 210, 48 };

static void submitScoreIfNeeded(Game *game) {
    if (game->scoreSubmitted || game->score <= 0) return;
    leaderboardAddScore(&game->leaderboard, game->playerName, game->score);
    game->scoreSubmitted = true;
}

static void updateMenu(Game *game) {
    if (!menuActionReady(game->menu)) return;

    switch (menuConsumeAction(game->menu)) {
    case MENU_ACTION_START:
        game->playerName[0] = '\0';
        game->currentState = STATE_NAME_ENTRY;
        break;
    case MENU_ACTION_LEADERBOARD:
        game->leaderboard.page = 0;
        game->currentState = STATE_LEADERBOARD;
        break;
    case MENU_ACTION_SETTING:
        game->currentState = STATE_SETTING;
        break;
    case MENU_ACTION_EXIT:
        game->quit = true;
        break;
    case MENU_ACTION_NONE:
        break;
    }
}

static void drawPause(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.63f));
    uiSurface((Rectangle){490, 310, 520, 250});
    uiTitle(game, "Flight paused", 527, 328, 55);
    uiLabel(game, "Flight time is suspended.", 530, 414, 24, UI_MUTED);
    uiButton(game, resumeButton, "Resume", CheckCollisionPointRec(GetMousePosition(), resumeButton));
    uiButton(game, leaveButton, "Abandon run", CheckCollisionPointRec(GetMousePosition(), leaveButton));
}

static void handleGameState(Game *game) {
    switch (game->currentState) {
    case STATE_MENU:
        runMenuPhysics(game);
        updateMenu(game);
        break;
    case STATE_NAME_ENTRY:
        updateNameEntry(game);
        if (IsKeyPressed(KEY_ESCAPE)) {
            PlaySound(game->menu->click_sfx);
            game->currentState = STATE_MENU;
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            PlaySound(game->menu->click_sfx);
            if (game->playerName[0] == '\0') strcpy(game->playerName, "PLAYER");
            game->currentState = STATE_GAME;
            resetRun(game);
            HideCursor();
        }
        break;
    case STATE_GAME:
        if (IsKeyPressed(KEY_M)) game->showRadar = !game->showRadar;
        if (IsKeyPressed(KEY_ESCAPE) && game->player.lives > 0 && !game->missionCleared && !game->timedOut) {
            ShowCursor();
            game->currentState = STATE_PAUSE;
            break;
        }
        runGamePhysics(game);
        if (game->outcomeTime >= 0.65f) {
            game->currentState = STATE_GAME_OVER;
            ShowCursor();
            ResumeMusicStream(game->menu->bg_song);
            submitScoreIfNeeded(game);
        }
        break;
    case STATE_GAME_OVER:
        updateGameOver(game);
        if (game->currentState == STATE_GAME) HideCursor();
        break;
    case STATE_LEADERBOARD:
        leaderboardUpdate(&game->leaderboard);
        if (leaderboardBack()) game->currentState = STATE_MENU;
        break;
    case STATE_PAUSE: {
        Vector2 pointer = GetMousePosition();
        bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
            (click && CheckCollisionPointRec(pointer, resumeButton))) {
            HideCursor();
            game->currentState = STATE_GAME;
        } else if (click && CheckCollisionPointRec(pointer, leaveButton)) {
            game->currentState = STATE_MENU;
            ResumeMusicStream(game->menu->bg_song);
        }
        break;
    }
    case STATE_SETTING:
        updateSettings(game);
        break;
    }
}

static Rectangle screenViewport(void) {
    float width = (float)GetScreenWidth();
    float height = (float)GetScreenHeight();
    float scale = fmaxf(0.001f, fminf(width / SCREEN_WIDTH, height / SCREEN_HEIGHT));
    return (Rectangle){ (width - SCREEN_WIDTH * scale) * 0.5f,
                         (height - SCREEN_HEIGHT * scale) * 0.5f,
                         SCREEN_WIDTH * scale, SCREEN_HEIGHT * scale };
}

static void renderFrame(Game *game, RenderTexture2D frame, Rectangle viewport) {
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
    EndMode2D();
    EndScissorMode();
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter | Sector Siege");
    if (!IsWindowReady()) return 1;
    SetWindowMinSize(640, 384);
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    if (monitorWidth > 0 && monitorHeight > 0) {
        float scale = fminf(1.0f, fminf((float)monitorWidth * 0.90f / SCREEN_WIDTH,
                                         (float)monitorHeight * 0.88f / SCREEN_HEIGHT));
        SetWindowSize((int)(SCREEN_WIDTH * scale), (int)(SCREEN_HEIGHT * scale));
    }
    SetExitKey(KEY_NULL);
    SetTargetFPS(120);
    InitAudioDevice();

    Game game;
    initializeGame(&game);
    loadSettings(&game);
    RenderTexture2D frame = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(frame.texture, TEXTURE_FILTER_BILINEAR);
    PlayMusicStream(game.menu->bg_song);

    while (!WindowShouldClose() && !game.quit) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        Rectangle viewport = screenViewport();
        float scale = viewport.width / SCREEN_WIDTH;
        SetMouseOffset(-(int)viewport.x, -(int)viewport.y);
        SetMouseScale(1.0f / scale, 1.0f / scale);
        handleGameState(&game);
        if (game.currentState != STATE_GAME && game.currentState != STATE_PAUSE) {
            UpdateMusicStream(game.menu->bg_song);
        }

        BeginDrawing();
        renderFrame(&game, frame, viewport);
        EndDrawing();
    }

    UnloadRenderTexture(frame);
    unloadGameAssets(&game);
    UnloadMusicStream(game.menu->bg_song);
    UnloadSound(game.menu->hover_sfx);
    UnloadSound(game.menu->click_sfx);
    free(game.menu);
    leaderboardShutdown(&game.leaderboard);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
