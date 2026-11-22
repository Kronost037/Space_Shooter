#include "core.h"
#include "game.h"
#include "menu.h"
#include "leaderboard.h"
#include "background.h"
#include "name_entry.h"
#include "game_over.h"

#include <string.h>

static const Rectangle volumeTrack = { 540, 410, 420, 15 };
static const Rectangle doneButton = { 640, 493, 220, 48 };
static const Rectangle resumeButton = { 527, 477, 210, 48 };
static const Rectangle leaveButton = { 763, 477, 210, 48 };
static float volume = 0.80f;

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
        game->leaderboard.animTimer = 0.0f;
        game->leaderboard.scroll = 0.0f;
        game->leaderboard.targetScroll = 0.0f;
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

static void updateSettings(Game *game) {
    Vector2 pointer = GetMousePosition();
    Rectangle hit = {volumeTrack.x, volumeTrack.y - 12.0f, volumeTrack.width, volumeTrack.height + 24.0f};
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(pointer, hit)) {
        volume = (pointer.x - volumeTrack.x) / volumeTrack.width;
    }
    if (IsKeyPressed(KEY_LEFT)) volume -= 0.05f;
    if (IsKeyPressed(KEY_RIGHT)) volume += 0.05f;
    volume = Clamp(volume, 0.0f, 1.0f);
    SetMasterVolume(volume);

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(pointer, doneButton))) {
        PlaySound(game->menu->click_sfx);
        game->currentState = STATE_MENU;
    }
}

static void drawSettings(const Game *game) {
    drawBackground(SCREEN_WIDTH, SCREEN_HEIGHT, (float)GetTime());
    Rectangle panel = { 490, 305, 520, 270 };
    Color cyan = {96, 222, 255, 255};
    DrawRectangleRounded(panel, 0.09f, 12, (Color){9, 15, 32, 245});
    DrawRectangleRoundedLines(panel, 0.09f, 12, Fade(cyan, 0.48f));
    DrawTextEx(game->font, "SETTINGS", (Vector2){540, 332}, 34, 1.2f, RAYWHITE);
    DrawText(TextFormat("MASTER VOLUME  %d%%", (int)roundf(volume * 100.0f)), 540, 381, 16, RAYWHITE);
    DrawRectangleRec(volumeTrack, (Color){39, 59, 94, 255});
    DrawRectangleRec((Rectangle){volumeTrack.x, volumeTrack.y,
                                 volumeTrack.width * volume, volumeTrack.height}, cyan);
    DrawText("LEFT / RIGHT OR CLICK THE BAR   ·   F11 FULLSCREEN", 540, 446, 15, (Color){161, 181, 215, 255});
    DrawRectangleRounded(doneButton, 0.20f, 8, (Color){22, 49, 75, 255});
    DrawText("BACK", 727, 508, 20, RAYWHITE);
}

static void drawPause(const Game *game) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.63f));
    Rectangle panel = { 490, 325, 520, 235 };
    DrawRectangleRounded(panel, 0.09f, 12, (Color){9, 15, 32, 248});
    DrawRectangleRoundedLines(panel, 0.09f, 12, (Color){67, 121, 172, 255});
    DrawTextEx(game->font, "PAUSED", (Vector2){527, 352}, 50, 1.1f, RAYWHITE);
    DrawText("ESC OR ENTER TO RESUME", 529, 423, 16, (Color){161, 181, 215, 255});
    DrawRectangleRounded(resumeButton, 0.16f, 8, (Color){22, 61, 84, 255});
    DrawRectangleRounded(leaveButton, 0.16f, 8, (Color){39, 42, 66, 255});
    DrawText("RESUME", 594, 492, 19, RAYWHITE);
    DrawText("ABANDON RUN", 805, 492, 19, RAYWHITE);
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
        if (IsKeyPressed(KEY_ESCAPE) && game->player.lives > 0 && !game->missionCleared) {
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
        if (IsKeyPressed(KEY_ESCAPE)) game->currentState = STATE_MENU;
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
    SetMasterVolume(volume);

    Game game;
    initializeGame(&game);
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

        BeginTextureMode(frame);
        ClearBackground(BLACK);
        switch (game.currentState) {
        case STATE_GAME:        drawGame(&game);                    break;
        case STATE_MENU:        drawMenu(&game);                    break;
        case STATE_NAME_ENTRY:  drawNameEntry(&game);               break;
        case STATE_GAME_OVER:   drawGameOver(&game);                break;
        case STATE_LEADERBOARD: drawLeaderboard(&game.leaderboard); break;
        case STATE_SETTING:     drawSettings(&game);                break;
        case STATE_PAUSE:
            drawGame(&game);
            drawPause(&game);
            break;
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(frame.texture, (Rectangle){0, 0, SCREEN_WIDTH, -SCREEN_HEIGHT},
                       viewport, (Vector2){0}, 0.0f, WHITE);
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
