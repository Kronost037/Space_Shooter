#ifndef GAME_H_
#define GAME_H_

#include "core.h"
#include <raylib.h>


#define SCREEN_WIDTH 1500
#define SCREEN_HEIGHT 900
#define PANEL_WIDTH 320
#define PLAYER_SCALE 0.03f
#define MAX_BULLETS 40
#define MAX_ENEMIES 20


typedef enum {
    STATE_MENU = 0,
    STATE_GAME,
    STATE_LEADERBOARD,
    STATE_SETTING,
} State;

typedef struct S_gamesound {
    Sound laser_sound;
} GameSound;

typedef struct S_projectile {
    bool active;
    Rectangle dim;
    float projectile_speed;
    Vector2 projectile_dir;
    Color projectile_color;
} Projectile;

typedef struct S_entity {
    bool active;
    Texture2D entity_texture;
    Vector2 entity_pos;
    float entity_speed;
    Vector2 entity_dir;
    Projectile entity_bullets[MAX_BULLETS];
    Vector2 entity_shooting_dir;
    float entity_shooting_cooldown;
    Color entity_color;
} Entity;

struct S_game {
    bool quit;
    GameSound gamesound;
    bool showPanel;
    int gameWidth;
    int gameHeight;
    State currentState;
    Menu *menu;
    Entity player;
    Entity enemies[MAX_ENEMIES];
    Font font;
    float timer;
    int enemiesKilled;
};

#endif
