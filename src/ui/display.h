#ifndef DISPLAY_H
#define DISPLAY_H

#include "../game.h"

extern const Rectangle resumeButton, leaveButton;
Rectangle screenViewport(void);
void renderFrame(Game *game, RenderTexture2D world, Rectangle viewport);

#endif
