#ifndef CINEMATIC_H
#define CINEMATIC_H

#include "../game.h"

void playScene(Game *game, FlightScene scene);
bool updateScene(Game *game, float dt, bool skip);
void drawScene(const Game *game);

#endif
