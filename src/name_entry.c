#include "name_entry.h"
#include "ui/widgets.h"

#include <string.h>

static const Rectangle launch = {756, 501, 315, 60};
static const Rectangle back = {430, 501, 305, 60};

bool nameEntryLaunch(void) {
    return uiClicked(launch) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
}

bool nameEntryBack(void) {
    return uiClicked(back) || IsKeyPressed(KEY_ESCAPE);
}

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
    uiSurface((Rectangle){378,295,744,317});
    uiTitle(game, "Callsign", 430, 332, 50);
    DrawRectangleRec((Rectangle){430,410,641,65}, (Color){24,41,55,255});
    const char *name = game->playerName;
    uiLabel(game, *name ? name : "PLAYER", 450, 424, 34, *name ? UI_PAPER : UI_MUTED);
    float end = *name ? 450 + uiTextWidth(game, name, 34) : 441;
    if (fmodf(t, 1.0f) < 0.6f) DrawRectangle((int)end + 3, 429, 2, 30, UI_ICE);
    uiButton(game, back, "Back", CheckCollisionPointRec(GetMousePosition(), back));
    uiButton(game, launch, "Launch", CheckCollisionPointRec(GetMousePosition(), launch));
}
