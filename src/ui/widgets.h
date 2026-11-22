#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include "../game.h"

#define UI_INK ((Color){12, 22, 34, 255})
#define UI_PAPER ((Color){240, 239, 226, 255})
#define UI_MUTED ((Color){177, 193, 204, 255})
#define UI_AMBER ((Color){255, 189, 99, 255})
#define UI_ICE ((Color){129, 221, 225, 255})

void uiBackdrop(const Game *game, float time);
void uiSurface(Rectangle area);
bool uiCompact(void);
float uiTextSize(float size);
float uiTextWidth(const Game *game, const char *text, float size);
void uiLabel(const Game *game, const char *text, float x, float y, float size, Color color);
void uiTitle(const Game *game, const char *text, float x, float y, float size);
void uiButton(const Game *game, Rectangle bounds, const char *label, bool selected);
bool uiClicked(Rectangle bounds);

#endif
