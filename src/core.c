#include "core.h"
#include "game.h"
#include "menu.h"
#include "leaderboard.h"

#include "name_entry.h"
#include "game_over.h"

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

static void submitScoreIfNeeded(Game *game) {
    if (game->scoreSubmitted || (game->enemiesKilled == 0 && game->leaderboard.count > 10)) return;

    const int max_entries = 500;
    
    if(game->leaderboard.count == max_entries && game->enemiesKilled < game->leaderboard.entries[max_entries - 1].score) return;

    const char *name = (game->playerName[0] != '\0') ? game->playerName : "PLAYER";
    leaderboardAddScore(&game->leaderboard, name, game->enemiesKilled);
    game->scoreSubmitted = true;
}


static void syncCameraLayout(Game *game) {
    game->camera.offset = (Vector2){
        game->gameWidth * 0.5f,
        game->gameHeight * 0.5f
    };
}

static void updateMenu(Game *game){
    if (!menuActionReady(game->menu)) return;

    switch(menuConsumeAction(game->menu)){

        case MENU_ACTION_START:
        game -> playerName[0] = '\0';
        game -> currentState = STATE_NAME_ENTRY;
        break;

        case MENU_ACTION_LEADERBOARD:
        game->leaderboard.animTimer = 0.0f;
        game->leaderboard.scroll = 0.0f;
        game->leaderboard.targetScroll = 0.0f;
        game->currentState = STATE_LEADERBOARD;
        break;

        case MENU_ACTION_EXIT:      game->quit = true;            break;
        case MENU_ACTION_SETTING:                                 break;
        case MENU_ACTION_NONE:                                    break;
    }
}

static void handleGameState(Game *game) {
    game->menu->mousePos = GetMousePosition();

    switch (game->currentState) {
    case STATE_MENU:  updateMenu(game);     break;
    case STATE_NAME_ENTRY:
        updateNameEntry(game);
        
        if (IsKeyPressed(KEY_ESCAPE)) {
            PlaySound(game->menu->click_sfx);
            game->playerName[0] = '\0';
            game->currentState = STATE_MENU;
            EnableCursor();
            ResumeMusicStream(game->menu->bg_song);
            return;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            PlaySound(game->menu->click_sfx);
            if (game->playerName[0] == '\0') {
                strcpy(game->playerName, "PLAYER");
            }
            game->currentState = STATE_GAME;
            resetRun(game);
        }
        break;

    case STATE_GAME:
        if (IsKeyPressed(KEY_M)) {
            game->showPanel = !game->showPanel;
            refreshLayout(game);
            syncCameraLayout(game);
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            game->currentState = STATE_MENU;
            EnableCursor();
            ResumeMusicStream(game->menu->bg_song);
        }

        if(game->player.lives == 0) {
            game->currentState = STATE_GAME_OVER;
            EnableCursor();
        }
        break;

    case STATE_GAME_OVER: {
        updateGameOver(game);
        break;
    }

    case STATE_LEADERBOARD:
        if (IsKeyPressed(KEY_ESCAPE)) game->currentState = STATE_MENU;
        break;

    default:
        fprintf(stderr, "UNREACHABLE: Not a valid game state.\n");
        break;
    }
}

static void unload(Game *game) {
    UnloadMusicStream(game->menu->bg_song);
    UnloadSound(game->menu->hover_sfx);
    UnloadSound(game->menu->click_sfx);
    UnloadFont(game->font);
    UnloadTexture(game->entity_texture.Enemy_1);
    UnloadTexture(game->entity_texture.Player);
    free(game->menu);
}

static void updateMusicForCurrentState(Game *game) {
    if (game->currentState == STATE_MENU ||
        game->currentState == STATE_GAME_OVER ||
        game->currentState == STATE_NAME_ENTRY ||
        game->currentState == STATE_LEADERBOARD) {
        UpdateMusicStream(game->menu->bg_song);
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetExitKey(KEY_NULL);

    InitAudioDevice();

    Game game = (Game){0};
    initializeGame(&game);

    PlayMusicStream(game.menu->bg_song);
    SetTargetFPS(60);
    
    while (!WindowShouldClose() && !game.quit) {
        handleGameState(&game);

        switch (game.currentState){

            case STATE_GAME_OVER:
            submitScoreIfNeeded(&game);
            ResumeMusicStream(game.menu->bg_song);
            break;

            case STATE_GAME:        runGamePhysics(&game);                   break;
            case STATE_MENU:        runMenuPhysics(&game);                   break;
            case STATE_LEADERBOARD: leaderboardUpdate(&game.leaderboard);    break;

            default:
            updateMusicForCurrentState(&game);
            break;

        }
        
    BeginDrawing();
    {
        ClearBackground(BLACK);

        switch (game.currentState) {
            
            case STATE_GAME:         drawGame(&game);                     break;
            case STATE_MENU:         drawMenu(&game);                     break;
            case STATE_NAME_ENTRY:   drawNameEntry(&game);                break;
            case STATE_GAME_OVER:    drawGameOver(&game);                 break;
            case STATE_LEADERBOARD:  drawLeaderboard(&game.leaderboard);  break;
            case STATE_SETTING:                                           break;
        }
        
    }
    EndDrawing();

    }
    
    unload(&game);
    leaderboardShutdown(&game.leaderboard);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
