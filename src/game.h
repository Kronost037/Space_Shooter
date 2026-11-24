#ifndef GAME_H_
#define GAME_H_

#include "core.h"
#include "leaderboard.h"
#include "rules.h"

#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1500
#define SCREEN_HEIGHT 900
#define MAX_ELITES 4
#define MAX_ENEMIES 24
#define MAX_LIVES 3
#define MAX_SHOTS 192
#define MAX_PARTICLES 160
#define WORLD_COLS 4
#define WORLD_ROWS 4
#define WORLD_SECTOR_WIDTH 1450
#define WORLD_SECTOR_HEIGHT 1200
#define WORLD_WIDTH (WORLD_COLS * WORLD_SECTOR_WIDTH)
#define WORLD_HEIGHT (WORLD_ROWS * WORLD_SECTOR_HEIGHT)
#define PULSE_RECHARGE 15.0f
#define PULSE_RADIUS 300.0f
#define SCOUT_REWARDS_PER_STAGE 12

typedef enum {
    STATE_MENU = 0,
    STATE_NAME_ENTRY,
    STATE_GAME,
    STATE_GAME_OVER,
    STATE_LEADERBOARD,
    STATE_SETTING,
    STATE_PAUSE,
} State;

typedef enum { ATTACK_FAN, ATTACK_LANCE, ATTACK_RING, ATTACK_LANES, ATTACK_PINCER } Attack;
typedef enum { ELITE_IDLE, ELITE_WARNING, ELITE_FIRING, ELITE_RECOVERY } ElitePhase;

typedef struct {
    Vector2 position;
    Vector2 previous;
    Vector2 velocity;
    float radius;
    float life;
    unsigned visibleTargets;
    int owner;                 // -1 for pilot shots, otherwise the elite's index.
} Shot;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float life;
    float totalLife;
    float size;
    Color color;
} Particle;

typedef struct {
    Vector2 position;
    Vector2 previous;
    Vector2 heading;
    float speed;
    float hitFlash;
    int lives;
} Ship;

typedef struct {
    Ship ship;
    ElitePhase phase;
    Attack attack;
    float time;
    float duration;
    float aim;
    int cycle;
    int maxLives;
    Vector2 patrolCenter;
    float patrolPhase;
    bool echo;
    bool unlocked;
} Elite;

typedef enum { MISSION_SURVEY, MISSION_RELAYS, MISSION_ESCORT, MISSION_CARRIER } MissionStage;

typedef enum { SCENE_NONE, SCENE_LAUNCH, SCENE_ESCORT, SCENE_CARRIER, SCENE_CLEAR } FlightScene;

enum { SURVEY_GATES = 3, ESCORT_LEGS = 2 };

typedef struct {
    MissionStage stage;
    int fixes, escortLeg;
    Vector2 escortRoute[ESCORT_LEGS];
    float loadingTime;
    Vector2 waypoint, transport;
    bool contested;
} Mission;

struct S_game {
    Mission mission;
    uint32_t spawnSeed;
    bool quit;
    State currentState;
    float screenAge;
    float sceneTime;
    FlightScene scene;
    Menu *menu;
    Font bodyFont;
    Settings settings;
    int settingRow;
    bool timedOut;
    Leaderboard leaderboard;
    bool scoreSubmitted;

    Texture2D sky, drydock, transportTexture, gateTexture;
    Vector2 velocity, thrust, gunAim, payoutPosition;
    const char *damageCause;
    Texture2D pilotTexture;
    Texture2D scoutTexture;
    Texture2D wardenTexture;
    Texture2D carrierTexture;
    Sound laserSound;
    Sound warningSound;
    Sound damageSound;
    Sound clearSound;
    Sound pulseSound;

    float hullAngle, previousHullAngle, turnSpeed;
    Camera2D camera;
    Vector2 previousCameraTarget;
    Ship player;
    Ship scouts[MAX_ENEMIES];
    Elite elites[MAX_ELITES];
    Shot shots[MAX_SHOTS];
    Particle particles[MAX_PARTICLES];
    char playerName[32];
    float accumulator;
    float outcomeTime;
    double elapsedTime;
    float fireCooldown;
    float spawnTimer;
    float playerHitCooldown;
    float pulseCharge;
    float pulseAnimation;
    float scoreToast;
    float dispatchTime;
    const char *dispatch;
    Vector2 pulseOrigin;
    int score;
    int lastAward;
    int scoutKills;
    int scoutRewards;
    int clearBonus;
    int wardensCleared;
    bool pulseQueued;
    bool missionCleared;
    bool showRadar;
};

void initializeGame(Game *game);
void shutdownGame(Game *game);
void advanceFlight(Game *game, double frameTime, Vector2 move, Vector2 aim, bool pulse);
void runGamePhysics(Game *game);
void drawGame(Game *game);
void resetRun(Game *game);
void resetRunSeeded(Game *game, uint32_t seed);

#endif
