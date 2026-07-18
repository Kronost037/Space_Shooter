#include "game_over.h"
#include "game.h"
#include "menu.h"
#include "leaderboard.h"
#include "background.h"
#include <stdio.h>

// Retain state variables privately to manage clean audio hover hooks
static bool prevHoverPlay = false;
static bool prevHoverMenu = false;

static Rectangle getButtonRect(Rectangle panel, int index) {
    const float buttonW = 220.0f;
    const float buttonH = 56.0f;
    const float gap = 20.0f;
    const float bottomPad = 34.0f;
    float total = buttonW * 2.0f + gap;
    return (Rectangle){
        panel.x + (panel.width - total) * 0.5f + index * (buttonW + gap),
        panel.y + panel.height - bottomPad - buttonH,
        buttonW,
        buttonH
    };
}

void updateGameOver(Game *game) {
    Rectangle panel = { ((float)GetScreenWidth() - 760.0f) * 0.5f, ((float)GetScreenHeight() - 400.0f) * 0.5f, 760.0f, 400.0f };
    Rectangle playAgain = getButtonRect(panel, 0);
    Rectangle mainMenu = getButtonRect(panel, 1);
    Vector2 mouse = GetMousePosition();
    
    bool hoverPlay = CheckCollisionPointRec(mouse, playAgain);
    bool hoverMenu = CheckCollisionPointRec(mouse, mainMenu);
    
    if (hoverPlay && !prevHoverPlay) PlaySound(game->menu->hover_sfx);
    if (hoverMenu && !prevHoverMenu) PlaySound(game->menu->hover_sfx);
    
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
        prevHoverPlay = prevHoverMenu = false;
    }
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
        PlaySound(game->menu->click_sfx);
        DisableCursor();
        game->currentState = STATE_GAME;
        resetRun(game);
        prevHoverPlay = prevHoverMenu = false;
    }
    
    if (IsKeyPressed(KEY_ESCAPE)) {
        PlaySound(game->menu->click_sfx);
        game->currentState = STATE_MENU;
        EnableCursor();
        prevHoverPlay = prevHoverMenu = false;
    }
}

void drawGameOver(Game *game) {
    float anim = (float)GetTime();
    drawBackground(GetScreenWidth(), GetScreenHeight(), anim);

    Rectangle panel = { ((float)GetScreenWidth() - 760.0f) * 0.5f, ((float)GetScreenHeight() - 400.0f) * 0.5f, 760.0f, 400.0f };
    DrawRectangleRounded(panel, 0.06f, 18, (Color){ 10, 15, 30, 220 });
    DrawRectangleRoundedLines(panel, 0.16f, 18, (Color){ 180, 0, 255, 170 });

    Font font = game->font;
    const char *title = "GAME OVER";
    Vector2 titleSize = MeasureTextEx(font, title, 42.0f, 1.0f);
    DrawTextEx(font, title, (Vector2){ panel.x + (panel.width - titleSize.x) * 0.5f, panel.y + 42.0f }, 42.0f, 1.0f, (Color){ 220, 240, 255, 255 });

    const char *name = (game->playerName[0] != '\0') ? game->playerName : "PLAYER";
    Vector2 nameSize = MeasureTextEx(font, name, 34.0f, 1.0f);
    DrawTextEx(font, name, (Vector2){ panel.x + (panel.width - nameSize.x) * 0.5f, panel.y + 140.0f }, 34.0f, 1.0f, (Color){ 0, 255, 255, 255 });
    
    char scoreBuf[64];
    snprintf(scoreBuf, sizeof(scoreBuf), "SCORE  %d", game->enemiesKilled);
    Vector2 scoreSize = MeasureTextEx(font, scoreBuf, 28.0f, 1.0f);
    DrawTextEx(font, scoreBuf, (Vector2){ panel.x + (panel.width - scoreSize.x) * 0.5f, panel.y + 200.0f }, 28.0f, 1.0f, (Color){ 220, 240, 255, 255 });

    // Render Button Layout Boundaries
    Rectangle playAgain = getButtonRect(panel, 0);
    Rectangle mainMenu = getButtonRect(panel, 1);
    Vector2 mouse = GetMousePosition();

    DrawRectangleRounded(playAgain, 0.06f, 18, CheckCollisionPointRec(mouse, playAgain) ? (Color){ 0, 40, 70, 230 } : (Color){ 5, 10, 18, 210 });
    DrawRectangleRounded(mainMenu, 0.06f, 18, CheckCollisionPointRec(mouse, mainMenu) ? (Color){ 30, 10, 50, 230 } : (Color){ 5, 10, 18, 210 });

    DrawTextEx(font, "PLAY AGAIN", (Vector2){ playAgain.x + 20.0f, playAgain.y + 16.0f }, 20.0f, 1.0f, WHITE);
    DrawTextEx(font, "MAIN MENU", (Vector2){ mainMenu.x + 25.0f, mainMenu.y + 16.0f }, 20.0f, 1.0f, WHITE);
}