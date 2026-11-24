#ifndef MISSION_H
#define MISSION_H

#include "../game.h"

void startMission(Game *game, Vector2 launch);
void beginEscort(Game *game, Vector2 relay);
void advanceMission(Game *game, float dt);
void drawTransport(Vector2 center, Vector2 heading, float scale);
void drawMission(const Game *game);
const char *missionTitle(const Game *game);
bool missionWaypoint(const Game *game, Vector2 *position);

#endif
