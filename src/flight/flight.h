#ifndef FLIGHT_H
#define FLIGHT_H

#include "../game.h"

#define STEP (1.0f / 120.0f)
#define PILOT_RADIUS 17.0f
#define SCOUT_RADIUS 23.0f
#define WARDEN_RADIUS 56.0f
#define CARRIER_RADIUS 73.0f
#define LANCE_WIDTH 17.0f
#define LANCE_REACH 1600.0f
#define CORAL ((Color){255, 86, 111, 255})
#define CYAN ((Color){96, 222, 255, 255})
#define AMBER ((Color){255, 194, 98, 255})
#define WARDEN_ACCENT ((Color){207, 144, 255, 255})

static inline Vector2 direction(float angle) {
    return (Vector2){ cosf(angle), sinf(angle) };
}

static inline bool isCarrier(int index) {
    return index == MAX_ELITES - 1;
}

static inline float eliteRadius(int index) {
    return isCarrier(index) ? CARRIER_RADIUS : WARDEN_RADIUS;
}

void burst(Game *game, Vector2 position, Color color, int count);
void award(Game *game, int points);
void damagePlayer(Game *game, const char *cause);
bool addShot(Game *game, int owner, Vector2 position, Vector2 heading, float speed, float radius, float life);
bool onScreen(Camera2D camera, Vector2 point, float margin);
float distanceToSegment(Vector2 point, Vector2 start, Vector2 end);
bool eliteCanAttack(const Game *game, int index);
void damageElite(Game *game, int index, int amount);
void updateElite(Game *game, int index);
void drawShip(Texture2D texture, Vector2 position, Vector2 heading, float size, Color tint, Color fallback);
void drawAttack(const Elite *elite, int index);

#endif
