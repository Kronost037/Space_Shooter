#include "name_entry.h"
#include "game.h"
#include "menu.h"
#include "background.h"

#include <string.h>
#include <math.h>


// File-scoped static configurations
static float backspaceHoldTime       = 0.0f; // why set the values here?
static float backspaceRepeatTimer    = 0.0f;

static bool isAllowedNameChar(int ch) {
    return ch >= 32 && ch <= 126;
}

void updateNameEntry(Game *game) {  // why not static?
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
        if (len > 0) game->playerName[len - 1] = '\0';
        
        backspaceHoldTime = 0.0f;
        backspaceRepeatTimer = 0.0f;
    }
    
    if (IsKeyDown(KEY_BACKSPACE)) {
        backspaceHoldTime += dt;
        
        if (backspaceHoldTime > 0.4f) {

            backspaceRepeatTimer += dt;
            
            if (backspaceRepeatTimer >= 0.06f) {
                size_t len = strlen(game->playerName);
                if (len > 0) game->playerName[len - 1] = '\0';
                
                backspaceRepeatTimer = 0.0f;
            }
        }
    } else {
        backspaceHoldTime = 0.0f;
        backspaceRepeatTimer = 0.0f;
    }
}

void drawNameEntry(Game *game) { // why not static
    float anim = (float)GetTime();
    drawBackground(GetScreenWidth(), GetScreenHeight(), anim);

    // what happened here??
    // Rectangle panel = makeCenteredPanel(760.0f, 420.0f);
    // drawGlassPanel(panel,
    //                (Color){ 10, 15, 30, 220 },
    //                (Color){ 0, 150, 255, 180 },
    //                (Color){ 0, 255, 255, 150 });

    // Layout math (relying on structural dimension extracted from core.c)
    Rectangle panel = {
        ((float)GetScreenWidth() - 760.0f) * 0.5f,
        ((float)GetScreenHeight() - 420.0f) * 0.5f,
        760.0f, 420.0f
    };

    // Draw semi-transparent UI Panels (calls out to Raylib drawing API directly)
    DrawRectangleRounded(panel, 0.06f, 18, (Color){ 10, 15, 30, 220 });
    DrawRectangleRoundedLines(panel, 0.16f, 18, (Color){ 0, 150, 255, 180 });

    Font font = game->font;

    const char *title = "CALLSIGN ENTRY";
    Vector2 titleSize = MeasureTextEx(font, title, 40.0f, 1.0f);
    Vector2 titlePos = {
        panel.x + (panel.width - titleSize.x) * 0.5f,
        panel.y + 42.0f
    };

    DrawTextEx(
        font, 
        title, 
        titlePos, 
        40.0f, 1.0f, 
        (Color){ 220, 240, 255, 255 }
    ); // ????

    const char *subtitle = "TYPE YOUR NAME AND PRESS ENTER";
    Vector2 subtitleSize = MeasureTextEx(font, subtitle, 18.0f, 1.0f);
    DrawTextEx(
        font,
        subtitle, 
        (Vector2){
            panel.x + (panel.width - subtitleSize.x) * 0.5f,
            panel.y + 96.0f
        }, 
        18.0f, 1.0f, 
        (Color){ 120, 150, 180, 255 }
    );

    Rectangle input = {
        panel.x + 48.0f,
        panel.y + 180.0f,
        panel.width - 96.0f,
        72.0f
    };

    DrawRectangleRounded(input, 0.06f, 18, (Color){ 5, 10, 18, 210 });
    DrawRectangleRoundedLines(input, 0.16f, 18, (Color){ 0, 255, 255, 170 });

    const char *display = (game->playerName[0] != '\0') ? game->playerName : "ENTER NAME";
    Color displayColor = (game->playerName[0] != '\0') ? (Color){ 220, 240, 255, 255 } : (Color){ 120, 150, 180, 180 };

    Vector2 displayPos = {
        input.x + 18.0f,
        input.y + (input.height - 34.0f) * 0.5f - 2.0f
    };
    DrawTextEx(font, display, displayPos, 34.0f, 1.0f,displayColor);


    // drawGlowText(font, title, titlePos, 40.0f, 1.0f,
    //              (Color){ 220, 240, 255, 255 },
    //              (Color){ 0, 255, 255, 200 });

    // const char *subtitle = "TYPE YOUR NAME AND PRESS ENTER";
    // Vector2 subtitleSize = MeasureTextEx(font, subtitle, 18.0f, 1.0f);
    

    
    // drawGlassPanel(input,
    //                (Color){ 5, 10, 18, 210 },
    //                (Color){ 0, 255, 255, 170 },
    //                (Color){ 180, 0, 255, 110 });

    

    float fontSize = 34.0f;
    // Vector2 displaySize = MeasureTextEx(font, display, fontSize, 1.0f);
    // Vector2 displayPos = {
    //     input.x + 18.0f,
    //     input.y + (input.height - displaySize.y) * 0.5f - 2.0f
    // };

    // drawGlowText(font, display, displayPos, fontSize, 1.0f,
    //              displayColor, (Color){ 0, 255, 255, 100 });

    
    
    
    if (fmodf(anim, 1.0f) < 0.5f) {
        float caretX = displayPos.x;

        if (game->playerName[0] != '\0') {
            Vector2 textSize = MeasureTextEx(font, game->playerName, fontSize, 1.0f);
            caretX += textSize.x + 2.0f;
        }

        DrawRectangleV(
            (Vector2){ caretX, input.y + 18.0f },
            (Vector2){ 2.0f, input.height - 36.0f },
            (Color){ 0, 255, 255, 220 });
    }


    // const char *limit = "MAX 20 CHARACTERS";
    // Vector2 limitSize = MeasureTextEx(font, limit, 16.0f, 1.0f);
    // DrawTextEx(font, limit, (Vector2){
    //     panel.x + (panel.width - limitSize.x) * 0.5f - 10.0f,
    //     panel.y + 356.0f
    // }, 18.0f, 2.0f, (Color){ 120, 150, 180, 220 });
}