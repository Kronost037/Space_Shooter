#include "game_over.h"
#include "menu.h"
#include "ui/widgets.h"

static const Rectangle retry = {356, 632, 375, 57};
static const Rectangle back = {756, 632, 375, 57};

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
    uiSurface((Rectangle){298,145,904,602});
    const char *title = game->missionCleared ? "Corridor open" : game->timedOut ? "Time up" : "Signal lost";
    uiTitle(game, title, 351, 239, 69);
    if (!game->missionCleared && !game->timedOut && game->damageCause)
        uiLabel(game, game->damageCause, 357, 332, 26, UI_MUTED);
    uiLabel(game, TextFormat("%s   /   %s", game->playerName, flightRules[game->settings.difficulty].name),
            356, 408, 27, UI_PAPER);
    uiTitle(game, TextFormat("%d points", game->score), 352, 448, 55);
    uiLabel(game, TextFormat("%d scouts     /     %d wardens     /     %d:%02d flight time",
            game->scoutKills, game->wardensCleared, (int)game->elapsedTime / 60,
            (int)game->elapsedTime % 60), 356, 540, 23, UI_MUTED);
    if (game->clearBonus)
        uiLabel(game, TextFormat("Quick-clear bonus: %d base points", game->clearBonus), 356, 580, 21, UI_ICE);
    uiButton(game, retry, "Retry", CheckCollisionPointRec(GetMousePosition(), retry));
    uiButton(game, back, "Main menu", CheckCollisionPointRec(GetMousePosition(), back));
}
