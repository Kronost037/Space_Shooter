#include "core.h"
#include "game.h"
#include "menu.h"
#include "leaderboard.h"
#include "ui/display.h"
#include "ui/cinematic.h"
#include "settings.h"
#include "name_entry.h"
#include "game_over.h"

#include <string.h>

static void submitScoreIfNeeded(Game *game) {
    if (game->scoreSubmitted || game->score <= 0) return;
    game->scoreSubmitted = leaderboardAddScore(&game->leaderboard, game->playerName, game->score);

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

static void handleGameState(Game *game) {
    switch (game->currentState) {
    case STATE_MENU:
        runMenuPhysics(game);
        updateMenu(game);
        break;
    case STATE_NAME_ENTRY:
        updateNameEntry(game);
        if (nameEntryBack()) {
            PlaySound(game->menu->click_sfx);
            game->currentState = STATE_MENU;
        } else if (nameEntryLaunch(game)) {
            PlaySound(game->menu->click_sfx);
            game->currentState = STATE_GAME;
            resetRun(game);
            playScene(game, SCENE_LAUNCH);
            HideCursor();
        }
        break;
    case STATE_GAME: {
        bool skip = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE);
        if (updateScene(game, GetFrameTime(), skip)) break;
        if (IsKeyPressed(KEY_M)) game->showRadar = !game->showRadar;
        if (IsKeyPressed(KEY_ESCAPE) && game->player.lives > 0 && !game->missionCleared && !game->timedOut) {
            ShowCursor();
            game->currentState = STATE_PAUSE;
            break;
        }
        MissionStage stage = game->mission.stage;
        bool cleared = game->missionCleared;
        runGamePhysics(game);
        if (!cleared && game->missionCleared) playScene(game, SCENE_CLEAR);
        else if (game->player.lives > 0 && !game->timedOut && stage != game->mission.stage) {
            if (game->mission.stage == MISSION_ESCORT) playScene(game, SCENE_ESCORT);
            if (game->mission.stage == MISSION_CARRIER) playScene(game, SCENE_CARRIER);
        }
        if (game->outcomeTime >= 0.65f) {
            game->currentState = STATE_GAME_OVER;
            ShowCursor();
            ResumeMusicStream(game->menu->bg_song);
            submitScoreIfNeeded(game);
        }
        break;
    }
    case STATE_GAME_OVER:
        updateGameOver(game);
        if (game->currentState == STATE_GAME) {
            playScene(game, SCENE_LAUNCH);
            HideCursor();
        }
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
        State previous = game.currentState;
        handleGameState(&game);
        game.screenAge = previous == game.currentState ? game.screenAge + GetFrameTime() : 0.0f;
        if (game.currentState != STATE_GAME && game.currentState != STATE_PAUSE) {
            UpdateMusicStream(game.menu->bg_song);
        }

        BeginDrawing();
        renderFrame(&game, frame, viewport);
        EndDrawing();
    }

    UnloadRenderTexture(frame);
    shutdownGame(&game);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
