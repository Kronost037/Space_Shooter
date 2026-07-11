#ifndef GAME_H_
#define GAME_H_

#include "core.h"
#include "leaderboard.h"

#include <raylib.h>
#include <stdlib.h>

#include <math.h>
#include "raymath.h"

#define SCREEN_WIDTH 1500
#define SCREEN_HEIGHT 900
#define PANEL_WIDTH 320
#define PLAYER_SCALE 0.03f
#define ENEMY_1_SCALE 0.3f

#define MAX_BULLETS 40
#define MAX_ENEMIES 20
#define MAX_LIVES 3

typedef enum {
    STATE_MENU = 0,
    STATE_NAME_ENTRY,
    STATE_GAME,
    STATE_GAME_OVER,
    STATE_LEADERBOARD,
    STATE_SETTING,
} State;

typedef struct S_gamesound {
    Sound laser_sound;
} GameSound;

typedef struct S_textures {
    Texture2D Player;
    Texture2D Enemy_1;
} Entity_Texture;

typedef struct S_projectile {
    bool active;
    Rectangle dim;
    float projectile_speed;
    Vector2 projectile_dir;
    Color projectile_color;
} Projectile;

typedef struct S_entity {
    int lives;
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
    Entity_Texture entity_texture;
    
    bool showPanel;
    int gameWidth;
    int gameHeight;
    State currentState;
    Font font;
    float timer;
    
    Menu *menu;
    
    Entity player;
    char playerName[32];
    float playerHitCooldown;

    Entity enemies[MAX_ENEMIES];
    int enemiesKilled;
    
    Leaderboard leaderboard;
    bool scoreSubmitted;
};


void initializeGame(Game *game);
void runGamePhysics(Game *game);
void drawGame(Game *game);


#endif
