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


#define MAX_ELITES 4
#define ELITE_ENEMY_SCALE (ENEMY_1_SCALE * 2.50f)
#define ELITE_ENEMY_LIVES 15
#define ELITE_BEAM_COOLDOWN 2.10f
#define ENEMY_SPEED_MULTIPLIER_START 0.75f
#define ENEMY_SPEED_MULTIPLIER_STEP 0.10f
#define ENEMY_SPEED_MULTIPLIER_MAX 1.50f


#define MAX_BULLETS 40
#define MAX_ENEMIES 20
#define MAX_LIVES 3

#define WORLD_COLS 4
#define WORLD_ROWS 4
#define WORLD_SECTOR_WIDTH 2200
#define WORLD_SECTOR_HEIGHT 2200

typedef struct S_world {
    int cols;
    int rows;
    int sectorWidth;
    int sectorHeight;
    int width;
    int height;
    int playerSectorX;
    int playerSectorY;
} World;

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

    float last_seen;
    
    Projectile entity_bullets[MAX_BULLETS];
    Vector2 entity_shooting_dir;
    float entity_shooting_cooldown;

    int sectorX;
    int sectorY;
} Entity;

struct S_game {
    bool quit;

    World world;
    Camera2D camera;
    
    GameSound gamesound;
    Entity_Texture entity_texture;
    
    bool showPanel;
    int gameWidth;
    int gameHeight;
    State currentState;
    Font font;
    float timer;
    float elapsedTime;
    
    Menu *menu;
    
    Entity player;
    char playerName[32];
    float playerHitCooldown;

    Entity enemies[MAX_ENEMIES];
    Entity elites[MAX_ELITES];

    float enemySpeedMultiplier;

    int enemiesKilled;
    Leaderboard leaderboard;
    bool scoreSubmitted;
};


void initializeGame(Game *game);
void runGamePhysics(Game *game);
void drawGame(Game *game);
void resetRun(Game *game);

#endif
