#include "core.h"
#include "game.h"
#include "menu.h"
#include "leaderboard.h"
#include "background.h"

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

static Rectangle makeCenteredPanel(float width, float height) {
    return (Rectangle){
        ((float)GetScreenWidth() - width) * 0.5f,
        ((float)GetScreenHeight() - height) * 0.5f,
        width,
        height
    };
}

static void drawNeonGlow(Rectangle rec, Color color, int layers) {
    for (int i = 0; i < layers; i++) {
        Rectangle g = {
            rec.x - (float)i,
            rec.y - (float)i,
            rec.width + (float)(i * 2),
            rec.height + (float)(i * 2)
        };
        Color c = color;
        c.a = (unsigned char)(color.a / (i + 1));
        DrawRectangleRoundedLines(g, 0.18f, 16, c);
    }
}

static void drawGlassPanel(Rectangle panel, Color fill, Color border, Color glow) {
    DrawRectangleRounded(panel, 0.06f, 18, fill);
    DrawRectangleRoundedLines(panel, 0.16f, 18, border);
    drawNeonGlow(panel, glow, 4);
}

static void drawGlowText(Font font, const char *text, Vector2 pos, float size, float spacing, Color textColor, Color glowColor) {
    for (int i = 3; i >= 1; i--) {
        Vector2 p = { pos.x + (float)i, pos.y + (float)i };
        Color c = glowColor;
        c.a = (unsigned char)(glowColor.a / (i + 1));
        DrawTextEx(font, text, p, size, spacing, c);
    }
    DrawTextEx(font, text, pos, size, spacing, textColor);
}

static Rectangle getButtonRect(Rectangle panel, int index) {
    const float buttonW = 220.0f;
    const float buttonH = 56.0f;
    const float gap = 20.0f;
    const float bottomPad = 34.0f;

    float total = buttonW * 2.0f + gap;
    float startX = panel.x + (panel.width - total) * 0.5f;
    float y = panel.y + panel.height - bottomPad - buttonH;

    return (Rectangle){
        startX + index * (buttonW + gap),
        y,
        buttonW,
        buttonH
    };
}

static void resetRun(Game *game) {
    game->lives = game->maxLives;
    game->playerHitCooldown = 0.0f;
    game->scoreSubmitted = false;
    game->enemiesKilled = 0;
    game->timer = 0.0f;
    game->showPanel = true;

    game->player.entity_pos = (Vector2){ 40.0f, 100.0f };
    game->player.entity_dir = (Vector2){ 0.0f, 0.0f };
    game->player.entity_shooting_dir = (Vector2){ 0.0f, -1.0f };
    game->player.entity_shooting_cooldown = 0.0f;

    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->player.entity_bullets, 0, sizeof(game->player.entity_bullets));

    refreshLayout(game);
    DisableCursor();
    PauseMusicStream(game->menu->bg_song);
}

static void submitScoreIfNeeded(Game *game) {
    if (game->scoreSubmitted) return;

    const char *name = (game->playerName[0] != '\0') ? game->playerName : "PLAYER";
    leaderboardAddScore(&game->leaderboard, name, game->enemiesKilled);
    printf("EMEMIES KILLED %d\n", game->enemiesKilled);
    game->scoreSubmitted = true;
}

static bool isAllowedNameChar(int ch) {
    return ch >= 32 && ch <= 126;
}

static float backspaceHoldTime;
static float backspaceRepeatTimer;

static void updateNameEntry(Game *game) {
    int ch = 0;
    while ((ch = GetCharPressed()) > 0) {
        size_t len = strlen(game->playerName);
        if (len < 20 && isAllowedNameChar(ch)) {
            game->playerName[len] = (char)ch;
            game->playerName[len + 1] = '\0';
        }
    }

    float dt = GetFrameTime();
    
    if (IsKeyPressed(KEY_BACKSPACE)) {
        size_t len = strlen(game->playerName);
        if (len > 0)
            game->playerName[len - 1] = '\0';
        
        backspaceHoldTime = 0.0f;
        backspaceRepeatTimer = 0.0f;
    }
    
    if (IsKeyDown(KEY_BACKSPACE)) {
        backspaceHoldTime += dt;
        
        if (backspaceHoldTime > 0.4f) {
            backspaceRepeatTimer += dt;
            
            if (backspaceRepeatTimer >= 0.06f) {
                size_t len = strlen(game->playerName);
                if (len > 0)
                    game->playerName[len - 1] = '\0';
                
                backspaceRepeatTimer = 0.0f;
            }
        }
    } else {
        backspaceHoldTime = 0.0f;
        backspaceRepeatTimer = 0.0f;
    }
}

static void drawNameEntry(Game *game) {
    float anim = (float)GetTime();
    drawBackground(GetScreenWidth(), GetScreenHeight(), anim);

    Rectangle panel = makeCenteredPanel(760.0f, 420.0f);
    drawGlassPanel(panel,
                   (Color){ 10, 15, 30, 220 },
                   (Color){ 0, 150, 255, 180 },
                   (Color){ 0, 255, 255, 150 });

    Font font = game->font;

    const char *title = "CALLSIGN ENTRY";
    Vector2 titleSize = MeasureTextEx(font, title, 40.0f, 1.0f);
    Vector2 titlePos = {
        panel.x + (panel.width - titleSize.x) * 0.5f,
        panel.y + 42.0f
    };
    drawGlowText(font, title, titlePos, 40.0f, 1.0f,
                 (Color){ 220, 240, 255, 255 },
                 (Color){ 0, 255, 255, 200 });

    const char *subtitle = "TYPE YOUR NAME AND PRESS ENTER";
    Vector2 subtitleSize = MeasureTextEx(font, subtitle, 18.0f, 1.0f);
    DrawTextEx(font, subtitle, (Vector2){
        panel.x + (panel.width - subtitleSize.x) * 0.5f,
        panel.y + 96.0f
    }, 18.0f, 1.0f, (Color){ 120, 150, 180, 255 });

    Rectangle input = {
        panel.x + 48.0f,
        panel.y + 180.0f,
        panel.width - 96.0f,
        72.0f
    };

    drawGlassPanel(input,
                   (Color){ 5, 10, 18, 210 },
                   (Color){ 0, 255, 255, 170 },
                   (Color){ 180, 0, 255, 110 });

    const char *display = (game->playerName[0] != '\0') ? game->playerName : "ENTER NAME";
    Color displayColor = (game->playerName[0] != '\0')
        ? (Color){ 220, 240, 255, 255 }
        : (Color){ 120, 150, 180, 180 };

    float fontSize = 34.0f;
    Vector2 displaySize = MeasureTextEx(font, display, fontSize, 1.0f);
    Vector2 displayPos = {
        input.x + 18.0f,
        input.y + (input.height - displaySize.y) * 0.5f - 2.0f
    };

    drawGlowText(font, display, displayPos, fontSize, 1.0f,
                 displayColor, (Color){ 0, 255, 255, 100 });

    if (fmodf(anim, 1.0f) < 0.5f) {
        float caretX = displayPos.x;

        if (game->playerName[0] != '\0') {
            Vector2 textSize = MeasureTextEx(font,
                                             game->playerName,
                                             fontSize,
                                             1.0f);
            caretX += textSize.x + 2.0f;
        }

        DrawRectangleV((Vector2){ caretX, input.y + 18.0f },
                       (Vector2){ 2.0f, input.height - 36.0f },
                       (Color){ 0, 255, 255, 220 });
    }


    const char *limit = "MAX 20 CHARACTERS";
    Vector2 limitSize = MeasureTextEx(font, limit, 16.0f, 1.0f);
    DrawTextEx(font, limit, (Vector2){
        panel.x + (panel.width - limitSize.x) * 0.5f - 10.0f,
        panel.y + 356.0f
    }, 18.0f, 2.0f, (Color){ 120, 150, 180, 220 });
}

static void drawGameOver(Game *game) {
    float anim = (float)GetTime();
    drawBackground(GetScreenWidth(), GetScreenHeight(), anim);

    Rectangle panel = makeCenteredPanel(760.0f, 400.0f);
    drawGlassPanel(panel,
                   (Color){ 10, 15, 30, 220 },
                   (Color){ 180, 0, 255, 170 },
                   (Color){ 0, 255, 255, 140 });

    Font font = game->font;

    const char *title = "GAME OVER";
    Vector2 titleSize = MeasureTextEx(font, title, 42.0f, 1.0f);
    drawGlowText(font, title, (Vector2){
        panel.x + (panel.width - titleSize.x) * 0.5f,
        panel.y + 42.0f
    }, 42.0f, 1.0f,
    (Color){ 220, 240, 255, 255 },
    (Color){ 180, 0, 255, 200 });

    const char *nameLabel = "OPERATIVE";
    Vector2 nameLabelSize = MeasureTextEx(font, nameLabel, 18.0f, 1.0f);
    DrawTextEx(font, nameLabel, (Vector2){
        panel.x + (panel.width - nameLabelSize.x) * 0.5f,
        panel.y + 120.0f
    }, 18.0f, 1.0f, (Color){ 120, 150, 180, 255 });

    const char *name = (game->playerName[0] != '\0') ? game->playerName : "PLAYER";
    Vector2 nameSize = MeasureTextEx(font, name, 34.0f, 1.0f);
    drawGlowText(font, name, (Vector2){
        panel.x + (panel.width - nameSize.x) * 0.5f,
        panel.y + 140.0f
    }, 34.0f, 1.0f,
    (Color){ 0, 255, 255, 255 },
    (Color){ 0, 255, 255, 120 });
    
    char scoreBuf[64];
    snprintf(scoreBuf, sizeof(scoreBuf), "SCORE  %d", game->enemiesKilled);

    Vector2 scoreSize = MeasureTextEx(font, scoreBuf, 28.0f, 1.0f);
    DrawTextEx(font, scoreBuf, (Vector2){
        panel.x + (panel.width - scoreSize.x) * 0.5f,
        panel.y + 200.0f
    }, 28.0f, 1.0f, (Color){ 220, 240, 255, 255 });

    Rectangle playAgain = getButtonRect(panel, 0);
    Rectangle mainMenu = getButtonRect(panel, 1);
    Vector2 mouse = GetMousePosition();

    bool hoverPlay = CheckCollisionPointRec(mouse, playAgain);
    bool hoverMenu = CheckCollisionPointRec(mouse, mainMenu);

    drawGlassPanel(playAgain,
                   hoverPlay ? (Color){ 0, 40, 70, 230 } : (Color){ 5, 10, 18, 210 },
                   hoverPlay ? (Color){ 0, 255, 255, 210 } : (Color){ 0, 150, 255, 150 },
                   hoverPlay ? (Color){ 0, 255, 255, 180 } : (Color){ 180, 0, 255, 100 });

    drawGlassPanel(mainMenu,
                   hoverMenu ? (Color){ 30, 10, 50, 230 } : (Color){ 5, 10, 18, 210 },
                   hoverMenu ? (Color){ 180, 0, 255, 210 } : (Color){ 0, 150, 255, 150 },
                   hoverMenu ? (Color){ 180, 0, 255, 180 } : (Color){ 0, 255, 255, 100 });

    const char *playText = "PLAY AGAIN";
    Vector2 playTextSize = MeasureTextEx(font, playText, 20.0f, 1.0f);
    drawGlowText(font, playText, (Vector2){
        playAgain.x + (playAgain.width - playTextSize.x) * 0.5f,
        playAgain.y + (playAgain.height - playTextSize.y) * 0.5f - 1.0f
    }, 20.0f, 1.0f,
    (Color){ 220, 240, 255, 255 },
    (Color){ 0, 255, 255, 100 });

    const char *menuText = "MAIN MENU";
    Vector2 menuTextSize = MeasureTextEx(font, menuText, 20.0f, 1.0f);
    drawGlowText(font, menuText, (Vector2){
        mainMenu.x + (mainMenu.width - menuTextSize.x) * 0.5f,
        mainMenu.y + (mainMenu.height - menuTextSize.y) * 0.5f - 1.0f
    }, 20.0f, 1.0f,
    (Color){ 220, 240, 255, 255 },
    (Color){ 180, 0, 255, 100 });
}

static void handleGameState(Game *game) {
    game->menu->mousePos = GetMousePosition();

    switch (game->currentState) {
    case STATE_MENU:
        if (menuActionReady(game->menu)) {
            MenuAction action = menuConsumeAction(game->menu);

            if (action == MENU_ACTION_START) {
                game->playerName[0] = '\0';
                game->currentState = STATE_NAME_ENTRY;
            } else if (action == MENU_ACTION_LEADERBOARD) {
                game->leaderboard.animTimer = 0.0f;
                game->leaderboard.scroll = 0.0f;
                game->leaderboard.targetScroll = 0.0f;
                game->currentState = STATE_LEADERBOARD;
            } else if (action == MENU_ACTION_SETTING) {
                /* reserved */
            } else if (action == MENU_ACTION_EXIT) {
                game->quit = true;
            }
        }
        break;

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
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            game->currentState = STATE_MENU;
            EnableCursor();
            ResumeMusicStream(game->menu->bg_song);
        }

        if(game->lives == 0) {
            game->currentState = STATE_GAME_OVER;
            EnableCursor();
        }
        break;

    case STATE_GAME_OVER: {
        static bool prevHoverPlay = false;
        static bool prevHoverMenu = false;
        
        Rectangle panel = makeCenteredPanel(760.0f, 400.0f);
        Rectangle playAgain = getButtonRect(panel, 0);
        Rectangle mainMenu = getButtonRect(panel, 1);
        Vector2 mouse = GetMousePosition();
        
        bool hoverPlay = CheckCollisionPointRec(mouse, playAgain);
        bool hoverMenu = CheckCollisionPointRec(mouse, mainMenu);
        
        if (hoverPlay && !prevHoverPlay) {
            PlaySound(game->menu->hover_sfx);
        }
        if (hoverMenu && !prevHoverMenu) {
            PlaySound(game->menu->hover_sfx);
        }
        
        prevHoverPlay = hoverPlay;
        prevHoverMenu = hoverMenu;
        
        if (hoverPlay && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            PlaySound(game->menu->click_sfx);
            game->currentState = STATE_GAME;
            resetRun(game);
            prevHoverPlay = prevHoverMenu = false;
        } else if (hoverMenu && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            PlaySound(game->menu->click_sfx);
            game->currentState = STATE_MENU;
            ResumeMusicStream(game->menu->bg_song);
            prevHoverPlay = prevHoverMenu = false;
        }
        
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            PlaySound(game->menu->click_sfx);
            DisableCursor();
            resetRun(game);
            prevHoverPlay = prevHoverMenu = false;
        }
        
        if (IsKeyPressed(KEY_ESCAPE)) {
            PlaySound(game->menu->click_sfx);
            game->currentState = STATE_MENU;
            EnableCursor();
            ResumeMusicStream(game->menu->bg_song);
            prevHoverPlay = prevHoverMenu = false;
        }
        break;
    }

    case STATE_LEADERBOARD:
        if (IsKeyPressed(KEY_ESCAPE)) {
            game->currentState = STATE_MENU;
        }
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
    UnloadTexture(game->player.entity_texture);
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

    game.maxLives = 3;
    game.lives = game.maxLives;
    game.playerName[0] = '\0';
    game.scoreSubmitted = false;
    game.playerHitCooldown = 0.0f;

    PlayMusicStream(game.menu->bg_song);
    SetTargetFPS(60);
    
    while (!WindowShouldClose() && !game.quit) {
        handleGameState(&game);
        
        if (game.currentState == STATE_GAME) {
            runGamePhysics(&game);
        } else if (game.currentState == STATE_GAME_OVER) {
            submitScoreIfNeeded(&game);
            ResumeMusicStream(game.menu->bg_song);
        } else if (game.currentState == STATE_MENU) {
            runMenuPhysics(&game);
        } else if (game.currentState == STATE_LEADERBOARD) {
            leaderboardUpdate(&game.leaderboard);
        } else {
            updateMusicForCurrentState(&game);
        }
        
    BeginDrawing();
    {
        ClearBackground(BLACK);
        
        if (game.currentState == STATE_GAME) {
            drawGame(&game);
        } else if (game.currentState == STATE_MENU) {
            drawMenu(&game);
        } else if (game.currentState == STATE_NAME_ENTRY) {
            drawNameEntry(&game);
        } else if (game.currentState == STATE_GAME_OVER) {
            drawGameOver(&game);
        } else if (game.currentState == STATE_LEADERBOARD) {
            drawLeaderboard(&game.leaderboard);
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
