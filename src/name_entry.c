#include "name_entry.h"
#include "ui/widgets.h"

#include <string.h>

void updateNameEntry(Game *game) {
    size_t length = strlen(game->playerName);
    int ch;
    while ((ch = GetCharPressed()) > 0) {
        if (length < 20 && ch >= 32 && ch <= 126) game->playerName[length++] = (char)ch;
    }
    if (length && (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))) --length;
    game->playerName[length] = '\0';
}

void drawNameEntry(Game *game) {
    float t = (float)GetTime();
    uiBackdrop(game, t);
    uiSurface((Rectangle){302,162,896,571});
    uiLabel(game, "TRANSMISSION   /   ASTER CONVOY", 361, 217, 21, UI_ICE);
    uiTitle(game, "The last crossing", 356, 251, 70);
    uiLabel(game, "Three Wardens are jamming the jump corridor.", 361, 351, 27, UI_PAPER);
    uiLabel(game, uiCompact() ? "Silence the relays to expose the Carrier." : "Silence their relays. The Carrier will have to break cover.", 361, 395, 25, UI_MUTED);
    uiLabel(game, "Pilot callsign", 361, 480, 23, UI_ICE);
    DrawRectangleRec((Rectangle){361,521,778,69}, (Color){24,41,55,255});
    const char *name = game->playerName;
    uiLabel(game, *name ? name : "PLAYER", 383, 534, 34, *name ? UI_PAPER : UI_MUTED);
    float end = 383 + uiTextWidth(game, name, 34);
    if (*name && fmodf(t, 1.0f) < 0.6f) DrawRectangle((int)end + 3, 541, 2, 30, UI_ICE);
    uiLabel(game, "Enter to launch     /     Esc to return", 361, 642, 24, UI_PAPER);
}
