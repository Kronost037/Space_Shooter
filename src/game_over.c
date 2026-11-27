#include "game_over.h"
#include "menu.h"
#include "ui/widgets.h"

static const Rectangle retry = {104, 553, 215, 59};
static const Rectangle back = {334, 553, 238, 59};

void updateGameOver(Game *game) {
    bool previous = game->debriefMenuSelected;
    if (Vector2LengthSqr(GetMouseDelta()) > 0) {
        Vector2 pointer = GetMousePosition();
        if (CheckCollisionPointRec(pointer, retry)) game->debriefMenuSelected = false;
        if (CheckCollisionPointRec(pointer, back)) game->debriefMenuSelected = true;
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP)) game->debriefMenuSelected = false;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN)) game->debriefMenuSelected = true;
    if (previous != game->debriefMenuSelected) PlaySound(game->menu.hover_sfx);

    bool activate = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
    if (uiClicked(back) || IsKeyPressed(KEY_ESCAPE) || (activate && game->debriefMenuSelected)) {
        PlaySound(game->menu.click_sfx);
        game->currentState = STATE_MENU;
    } else if (uiClicked(retry) || (activate && !game->debriefMenuSelected)) {
        PlaySound(game->menu.click_sfx);
        game->currentState = STATE_GAME;
        resetRun(game);
    }
}

void drawGameOver(Game *game) {
    uiBackdrop(game, (float)GetTime());
    const char *title = game->missionCleared ? "Corridor open" : game->timedOut ? "Time up" : "Signal lost";
    DrawRectangleGradientH(0, 0, 980, SCREEN_HEIGHT, (Color){5,12,19,220}, (Color){5,12,19,0});
    uiLabel(game, title, 108, 225, 32, game->missionCleared ? UI_ICE : UI_AMBER);
    uiTitle(game, TextFormat("%d", game->score), 100, 273, 108);
    uiLabel(game, "points", 110 + uiTextWidth(game, TextFormat("%d", game->score), 108),
            336, 28, UI_MUTED);
    uiLabel(game, game->playerName, 108, 420, 34, UI_PAPER);
    uiLabel(game, flightRules[game->settings.difficulty].name, 108, 468, 25, UI_MUTED);
    if (game->score > 0 && !game->scoreSubmitted)
        uiLabel(game, "Record could not be saved", 108, 510, 21, UI_AMBER);
    uiAction(game, retry, "Fly again", !game->debriefMenuSelected);
    uiAction(game, back, "Main menu", game->debriefMenuSelected);
}
