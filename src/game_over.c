#include "game_over.h"
#include "menu.h"
#include "ui/widgets.h"

static const Rectangle retry = {104, 553, 215, 59};
static const Rectangle back = {334, 553, 238, 59};

void updateGameOver(Game *game) {
    if (uiClicked(back) || IsKeyPressed(KEY_ESCAPE)) {
        PlaySound(game->menu->click_sfx);
        game->currentState = STATE_MENU;
    } else if (uiClicked(retry) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
        PlaySound(game->menu->click_sfx);
        game->currentState = STATE_GAME;
        resetRun(game);
    }
}

void drawGameOver(Game *game) {
    uiBackdrop(game, (float)GetTime());
    const char *title = game->missionCleared ? "Corridor open" : game->timedOut ? "Time up" : "Signal lost";
    uiLabel(game, title, 108, 244, 37, game->missionCleared ? UI_ICE : UI_AMBER);
    uiTitle(game, TextFormat("%d", game->score), 98, 291, 125);
    uiLabel(game, "points", 109, 421, 27, UI_MUTED);
    if (!game->missionCleared && !game->timedOut && game->damageCause)
        uiLabel(game, game->damageCause, 109, 478, 23, UI_MUTED);
    bool hoverBack = CheckCollisionPointRec(GetMousePosition(), back);
    uiAction(game, retry, "Fly again", !hoverBack);
    uiAction(game, back, "Main menu", hoverBack);
}
