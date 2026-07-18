#include "game.h"
#include "leaderboard.h"
#include "menu.h"
#include "panel.h"
#include "background.h"

#include "raymath.h"
#include <math.h>
#include <raylib.h>


#define ELITE_BEAM_SPEED 1100.0f
#define ELITE_BEAM_THICKNESS 8.0f
#define ELITE_BEAM_LENGTH 22.0f
#define ELITE_BEAM_RETRY 0.25f
#define ELITE_BEAM_WIDTH 50.0f

static float enemySpawnTimer = 0.0f;
static bool eliteDeathAwarded[MAX_ELITES] = { 0 };

static Vector2 getScaledTextureSize(Texture2D texture, float scale) {
    return (Vector2){
        (float)texture.width * scale,
        (float)texture.height * scale
    };
}


static Rectangle getEntityRect(const Entity *entity, const Texture2D texture, float scale) {
    Vector2 scaled = getScaledTextureSize(texture, scale);
    return (Rectangle){
        entity->entity_pos.x,
        entity->entity_pos.y,
        scaled.x,
        scaled.y
    };
}

static Vector2 getEntityCenter(const Entity *entity, const Texture2D texture, float scale) {
    
    Rectangle r = getEntityRect(entity, texture, scale);
    
    return (Vector2){ r.x + r.width * 0.5f, r.y + r.height * 0.5f };
}


static float clampf(float v, float lo, float hi) {
    return fmaxf(lo, fminf(v, hi));
}

static int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static Vector2 normalizeOrZero(Vector2 v) {
    float lenSq = Vector2LengthSqr(v);
    if (lenSq <= 0.000001f) return (Vector2){ 0.0f, 0.0f };
    return Vector2Scale(v, 1.0f / sqrtf(lenSq));
}

static int getSectorX(const Game *game, float x) {
    return clampi((int)(x / (float)game->world.sectorWidth), 0, game->world.cols - 1);
}

static int getSectorY(const Game *game, float y) {
    return clampi((int)(y / (float)game->world.sectorHeight), 0, game->world.rows - 1);
}

static void updatePlayerSector(Game *game) {
    Vector2 playerCenter = getEntityCenter(&game->player, game->entity_texture.Player, PLAYER_SCALE);
    game->world.playerSectorX = getSectorX(game, playerCenter.x);
    game->world.playerSectorY = getSectorY(game, playerCenter.y);
    game->player.sectorX = game->world.playerSectorX;
    game->player.sectorY = game->world.playerSectorY;
}

static float pointSegmentDistanceSq(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 ab = Vector2Subtract(b, a);
    float abLenSq = Vector2LengthSqr(ab);
    if (abLenSq <= 0.000001f) return Vector2LengthSqr(Vector2Subtract(p, a));

    Vector2 ap = Vector2Subtract(p, a);
    float t = (ap.x * ab.x + ap.y * ab.y) / abLenSq;
    t = clampf(t, 0.0f, 1.0f);

    Vector2 closest = Vector2Add(a, Vector2Scale(ab, t));
    return Vector2LengthSqr(Vector2Subtract(p, closest));
}

static bool entityInSector(const Game *game, const Entity *entity, Texture2D texture, float scale, int sx, int sy) {
    Vector2 center = getEntityCenter(entity, texture, scale);
    return getSectorX(game, center.x) == sx && getSectorY(game, center.y) == sy;
}

static bool eliteBeamBlocked(Game *game, const Entity *elite, Vector2 eliteCenter, Vector2 playerCenter) {
    int sx = elite->sectorX;
    int sy = elite->sectorY;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (enemy->lives <= 0) continue;
        if (!entityInSector(game, enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE, sx, sy)) continue;

        Vector2 enemyCenter = getEntityCenter(enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        if (pointSegmentDistanceSq(enemyCenter, eliteCenter, playerCenter) <= ELITE_BEAM_WIDTH * ELITE_BEAM_WIDTH) {
            return true;
        }
    }

    for (int i = 0; i < MAX_ELITES; i++) {
        Entity *other = &game->elites[i];
        if (other == elite) continue;
        if (other->lives <= 0) continue;
        if (!entityInSector(game, other, game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE, sx, sy)) continue;

        Vector2 otherCenter = getEntityCenter(other, game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE);
        if (pointSegmentDistanceSq(otherCenter, eliteCenter, playerCenter) <= ELITE_BEAM_WIDTH * ELITE_BEAM_WIDTH) {
            return true;
        }
    }

    return false;
}

static void awardEliteDeath(Game *game, int eliteIndex) {
    if (eliteDeathAwarded[eliteIndex]) return;

    eliteDeathAwarded[eliteIndex] = true;
    game->enemiesKilled += 50;
    game->enemySpeedMultiplier += ENEMY_SPEED_MULTIPLIER_STEP;
    if (game->enemySpeedMultiplier > ENEMY_SPEED_MULTIPLIER_MAX) {
        game->enemySpeedMultiplier = ENEMY_SPEED_MULTIPLIER_MAX;
    }

    for (int j = 0; j < MAX_BULLETS; j++) {
        game->elites[eliteIndex].entity_bullets[j].active = false;
    }
}

static Rectangle getSectorRect(const Game *game, int sx, int sy) {
    return (Rectangle){
        (float)(sx * game->world.sectorWidth),
        (float)(sy * game->world.sectorHeight),
        (float)game->world.sectorWidth,
        (float)game->world.sectorHeight
    };
}

static void spawnElites(Game *game) {
    int sectors[WORLD_COLS * WORLD_ROWS][2];
    int count = 0;

    for (int y = 0; y < game->world.rows; y++) {
        for (int x = 0; x < game->world.cols; x++) {
            if (x == game->world.playerSectorX && y == game->world.playerSectorY) continue;
            sectors[count][0] = x;
            sectors[count][1] = y;
            count++;
        }
    }

    for (int i = count - 1; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int tx = sectors[i][0];
        int ty = sectors[i][1];
        sectors[i][0] = sectors[j][0];
        sectors[i][1] = sectors[j][1];
        sectors[j][0] = tx;
        sectors[j][1] = ty;
    }

    for (int i = 0; i < MAX_ELITES; i++) {
        Entity *elite = &game->elites[i];
        *elite = (Entity){0};

        elite->lives = ELITE_ENEMY_LIVES;
        elite->entity_speed = 0.0f;
        elite->entity_dir = (Vector2){ 0.0f, 0.0f };
        elite->entity_shooting_cooldown = (float)GetRandomValue(0, 100) / 100.0f * ELITE_BEAM_COOLDOWN;
        elite->sectorX = sectors[i][0];
        elite->sectorY = sectors[i][1];

        Vector2 size = getScaledTextureSize(game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE);
        Rectangle sector = getSectorRect(game, elite->sectorX, elite->sectorY);

        elite->entity_pos = (Vector2){
            sector.x + sector.width * 0.5f - size.x * 0.5f,
            sector.y + sector.height * 0.5f - size.y * 0.5f
        };
    }
}

static void fireEliteBeam(Game *game, Entity *elite) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (elite->entity_bullets[i].active) return;
    }

    Vector2 eliteCenter = getEntityCenter(elite, game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE);
    Vector2 playerCenter = getEntityCenter(&game->player, game->entity_texture.Player, PLAYER_SCALE);
    Vector2 dir = normalizeOrZero(Vector2Subtract(playerCenter, eliteCenter));

    if (Vector2LengthSqr(dir) <= 0.000001f) return;

    for (int i = 0; i < MAX_BULLETS; i++) {
        Projectile *beam = &elite->entity_bullets[i];
        if (beam->active) continue;

        beam->active = true;
        beam->dim = (Rectangle){
            eliteCenter.x - ELITE_BEAM_THICKNESS * 0.5f,
            eliteCenter.y - ELITE_BEAM_LENGTH * 0.5f,
            ELITE_BEAM_THICKNESS,
            ELITE_BEAM_LENGTH
        };
        beam->projectile_speed = ELITE_BEAM_SPEED;
        beam->projectile_dir = dir;
        beam->projectile_color = GREEN;
        break;
    }
}

static void initializePlayer(Entity *player) {
    *player = (Entity){0};
    player->lives = MAX_LIVES;
    player->entity_pos = (Vector2){ 40.0f, 100.0f };
    player->entity_speed = 600.0f;
    player->entity_dir = (Vector2){0};
    player->entity_shooting_dir = (Vector2){ 0.0f, -1.0f };
    player->entity_shooting_cooldown = 0.0f;
    player->sectorX = 0;
    player->sectorY = 0;
}


static void syncCameraLayout(Game *game) {
    game->camera.offset = (Vector2){
        (float)game->gameWidth * 0.5f,
        (float)game->gameHeight * 0.5f
    };
}

static void initializeWorld(Game *game) {
    game->world.cols          = WORLD_COLS;
    game->world.rows          = WORLD_ROWS;
    game->world.sectorWidth   = WORLD_SECTOR_WIDTH;
    game->world.sectorHeight  = WORLD_SECTOR_HEIGHT;
    game->world.width         = WORLD_COLS * WORLD_SECTOR_WIDTH;
    game->world.height        = WORLD_ROWS * WORLD_SECTOR_HEIGHT;
    game->world.playerSectorX = 0;
    game->world.playerSectorY = 0;
}

static Vector2 getRandomPointInsideRect(Rectangle r, Vector2 size) {
    float maxX = r.x + r.width - size.x;
    float maxY = r.y + r.height - size.y;

    return (Vector2){
        (float)GetRandomValue((int)r.x, (int)fmaxf(r.x, maxX)),
        (float)GetRandomValue((int)r.y, (int)fmaxf(r.y, maxY))
    };
}

static void spawnPlayerInRandomSector(Game *game) {
    int sx = GetRandomValue(0, game->world.cols - 1);
    int sy = GetRandomValue(0, game->world.rows - 1);

    game->world.playerSectorX = sx;
    game->world.playerSectorY = sy;

    Vector2 playerSize = getScaledTextureSize(game->entity_texture.Player, PLAYER_SCALE);
    Rectangle sector = getSectorRect(game, sx, sy);

    game->player.entity_pos = getRandomPointInsideRect(sector, playerSize);
    game->player.sectorX = sx;
    game->player.sectorY = sy;
}

static void spawnEnemy(Game *game) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (enemy->lives > 0) continue;

        int sx = 0;
        int sy = 0;

        for (int tries = 0; tries < 32; tries++) {
            sx = GetRandomValue(0, game->world.cols - 1);
            sy = GetRandomValue(0, game->world.rows - 1);

            if (sx != game->world.playerSectorX || sy != game->world.playerSectorY) break;
        }

        Vector2 enemySize = getScaledTextureSize(game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        Rectangle sector = getSectorRect(game, sx, sy);
        Vector2 pos = getRandomPointInsideRect(sector, enemySize);

        *enemy              = (Entity){0};
        enemy->lives        = 1;
        enemy->entity_speed = (float)GetRandomValue(250, 350);
        enemy->entity_pos = pos;
        enemy->entity_dir = (Vector2){ 0.0f, 0.0f };
        enemy->entity_shooting_dir = (Vector2){ 0.0f, 0.0f };
        enemy->entity_shooting_cooldown = 0.0f;
        enemy->sectorX = sx;
        enemy->sectorY = sy;
        enemy->last_seen = -1000000.0f;
        break;
    }
}

static bool enemyVisible(Game *game, Entity *enemy)
{
    Vector2 screen =
        GetWorldToScreen2D(enemy->entity_pos, game->camera);

    Vector2 size =
        getScaledTextureSize(
            game->entity_texture.Enemy_1,
            ENEMY_1_SCALE
        );

    return
        screen.x + size.x >= 0 &&
        screen.y + size.y >= 0 &&
        screen.x <= game->gameWidth &&
        screen.y <= game->gameHeight;
}

static void clampPlayerToWorld(Game *game) {
    Entity *player = &game->player;
    Vector2 playerSize = getScaledTextureSize(game->entity_texture.Player, PLAYER_SCALE);

    float maxX = (float)game->world.width - playerSize.x;
    float maxY = (float)game->world.height - playerSize.y;

    if (player->entity_pos.x < 0.0f) player->entity_pos.x = 0.0f;
    if (player->entity_pos.y < 0.0f) player->entity_pos.y = 0.0f;
    if (player->entity_pos.x > maxX) player->entity_pos.x = maxX;
    if (player->entity_pos.y > maxY) player->entity_pos.y = maxY;
}

static void updateCamera(Game *game) {
    Vector2 target = getEntityCenter(&game->player, game->entity_texture.Player, PLAYER_SCALE);

    float lerp = 1.0f - expf(-8.0f * GetFrameTime());
    game->camera.target = Vector2Lerp(game->camera.target, target, lerp);

    float halfW = ((float)game->gameWidth * 0.5f) / game->camera.zoom;
    float halfH = ((float)game->gameHeight * 0.5f) / game->camera.zoom;

    if ((float)game->world.width <= halfW * 2.0f) {
        game->camera.target.x = (float)game->world.width * 0.5f;
    } else {
        game->camera.target.x = fmaxf(halfW, fminf(game->camera.target.x, (float)game->world.width - halfW));
    }

    if ((float)game->world.height <= halfH * 2.0f) {
        game->camera.target.y = (float)game->world.height * 0.5f;
    } else {
        game->camera.target.y = fmaxf(halfH, fminf(game->camera.target.y, (float)game->world.height - halfH));
    }
}

void resetRun(Game *game) {
    game->player.lives = MAX_LIVES;
    game->playerHitCooldown = 0.0f;
    game->scoreSubmitted = false;
    game->enemiesKilled = 0;
    game->timer = 0.0f;
    game->elapsedTime = 0.0f;
    game->showPanel = true;
    game->enemySpeedMultiplier = ENEMY_SPEED_MULTIPLIER_START;

    game->player.entity_dir = (Vector2){ 0.0f, 0.0f };
    game->player.entity_shooting_dir = (Vector2){ 0.0f, -1.0f };
    game->player.entity_shooting_cooldown = 0.0f;

    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->elites, 0, sizeof(game->elites));
    memset(game->player.entity_bullets, 0, sizeof(game->player.entity_bullets));
    memset(eliteDeathAwarded, 0, sizeof(eliteDeathAwarded));

    enemySpawnTimer = 0.0f;

    refreshLayout(game);
    spawnPlayerInRandomSector(game);
    spawnElites(game);

    game->camera.target = getEntityCenter(&game->player, game->entity_texture.Player, PLAYER_SCALE);

    DisableCursor();
    PauseMusicStream(game->menu->bg_song);
}
static void initializeSound(GameSound *gamesound) {
    gamesound->laser_sound = LoadSound("src/Assets/laser_shot.mp3");
    SetSoundVolume(gamesound->laser_sound, 0.1f);
}

void initializeGame(Game *game) {
    *game               = (Game){0};
    game->quit          = false;
    game->showPanel     = true;
    game->currentState  = STATE_MENU;
    game->timer         = 0.0f;
    game->elapsedTime   = 0.0f;
    game->enemiesKilled = 0;
    game->enemySpeedMultiplier = ENEMY_SPEED_MULTIPLIER_START;

    game->player.lives      = MAX_LIVES;
    game->playerName[0]     = '\0';
    game->scoreSubmitted    = false;
    game->playerHitCooldown = 0.0f;

    game->entity_texture.Player = LoadTexture("src/Assets/ufo.png");
    game->entity_texture.Enemy_1 = LoadTexture("src/Assets/enemy_1.png");

    refreshLayout(game);
    initializeWorld(game);

    game->camera = (Camera2D){0};
    game->camera.zoom = 1.0f;

    game->camera.rotation = 0.0f;
    syncCameraLayout(game);

    initializePlayer(&game->player);
    spawnPlayerInRandomSector(game);

    game->menu = malloc(sizeof *game->menu);
    initializeMenu(game->menu);

    for (int i = 0; i < MAX_ENEMIES; i++) game->enemies[i] = (Entity){0};
    for (int i = 0; i < MAX_ELITES; i++) game->elites[i] = (Entity){0};

    initializeSound(&game->gamesound);
    game->font = LoadFontEx("src/Assets/font/KnightWarrior.otf", 40, NULL, 0);

    game->menu->startButton = (Button){ .bounds = {0}, .label = "START" };
    game->menu->exitButton = (Button){ .bounds = {0}, .label = "EXIT" };

    leaderboardInit(&game->leaderboard);
    leaderboardLoad(&game->leaderboard);

    spawnElites(game);
}

static bool getShootingDirection(Vector2 *dir) {
    *dir = (Vector2){0};
    bool shot = false;

    if (IsKeyDown(KEY_D)) { dir->x += 1.0f; shot = true; }
    if (IsKeyDown(KEY_A)) { dir->x -= 1.0f; shot = true; }
    if (IsKeyDown(KEY_S)) { dir->y += 1.0f; shot = true; }
    if (IsKeyDown(KEY_W)) { dir->y -= 1.0f; shot = true; }

    if (dir->x == 0.0f && dir->y == 0.0f) return false;

    *dir = Vector2Normalize(*dir);
    return shot;
}

static Vector2 getPlayerDirection(void) {
    Vector2 entity_dir = {0};

    if (IsKeyDown(KEY_RIGHT)) entity_dir.x += 1.0f;
    if (IsKeyDown(KEY_LEFT))  entity_dir.x -= 1.0f;
    if (IsKeyDown(KEY_DOWN))  entity_dir.y += 1.0f;
    if (IsKeyDown(KEY_UP))    entity_dir.y -= 1.0f;

    if (entity_dir.x == 0.0f && entity_dir.y == 0.0f) return entity_dir;
    return Vector2Normalize(entity_dir);
}

static void makeBullet(Game *game) {
    Entity *player = &game->player;
    
    Vector2 playerSize = getScaledTextureSize(game->entity_texture.Player, PLAYER_SCALE);

    for (int i = 0; i < MAX_BULLETS; i++) {
        Projectile *bullet = &player->entity_bullets[i];
        if (!bullet->active) {
            bullet->dim = (Rectangle){
                player->entity_pos.x + playerSize.x * 0.5f - 2.0f,
                player->entity_pos.y + playerSize.y * 0.5f - 10.0f,
                4.0f,
                20.0f
            };
            bullet->projectile_dir   = player->entity_shooting_dir;
            bullet->projectile_speed = 800.0f;
            bullet->projectile_color = RED;
            bullet->active           = true;
            break;
        }
    }
}

//-------------------------------------------------------------------------------

//-------------DSR----------------

static void updateEnemies(Game *game) {
    float dt = GetFrameTime();

    enemySpawnTimer += dt;
    if (enemySpawnTimer >= 2.0f) {
        spawnEnemy(game);
        enemySpawnTimer = 0.0f;
    }

    Entity *player = &game->player;
    Vector2 playerCenter = getEntityCenter(player, game->entity_texture.Player, PLAYER_SCALE);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (enemy->lives == 0) continue;

        Vector2 enemyCenter = getEntityCenter(enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        Vector2 toPlayer = Vector2Subtract(playerCenter, enemyCenter);
        float distSq = Vector2LengthSqr(toPlayer);
        float dist = sqrtf(distSq);

        Vector2 desiredDir = normalizeOrZero(toPlayer);

        Vector2 separation = { 0.0f, 0.0f };
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (i == j) continue;

            Entity *other = &game->enemies[j];
            if (other->lives == 0) continue;

            Vector2 otherCenter = getEntityCenter(other, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
            Vector2 away = Vector2Subtract(enemyCenter, otherCenter);
            float awaySq = Vector2LengthSqr(away);

            if (awaySq > 0.001f && awaySq < 160.0f * 160.0f) {
                separation = Vector2Add(separation, Vector2Scale(away, 1.0f / awaySq));
            }
        }

        separation = normalizeOrZero(separation);

        Vector2 steer = Vector2Add(desiredDir, Vector2Scale(separation, 0.75f));
        steer = normalizeOrZero(steer);

        float smooth = 1.0f - expf(-7.0f * dt);
        enemy->entity_dir = Vector2Lerp(enemy->entity_dir, steer, smooth);
        enemy->entity_dir = normalizeOrZero(enemy->entity_dir);

        float slowRadius = 220.0f;
        float speedScale = 0.45f + 0.55f * fminf(dist / slowRadius, 1.0f);

        enemy->entity_pos = Vector2Add(
            enemy->entity_pos,
            Vector2Scale(enemy->entity_dir, enemy->entity_speed * game->enemySpeedMultiplier * speedScale * dt)
        );

        if (enemyVisible(game, enemy)) {
            enemy->last_seen = game->elapsedTime;
        }

        Rectangle enemyRec  = getEntityRect(enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        Rectangle playerRec = getEntityRect(player, game->entity_texture.Player, PLAYER_SCALE);

        if (CheckCollisionRecs(enemyRec, playerRec)) {
            if (game->playerHitCooldown <= 0.0f) {
                game->player.lives--;
                game->playerHitCooldown = 1.75f;
            }
            continue;
        }

        for (int j = 0; j < MAX_BULLETS; j++) {
            Projectile *bullet = &player->entity_bullets[j];
            if (!bullet->active) continue;

            if (CheckCollisionRecs(enemyRec, bullet->dim)) {
                enemy->lives = 0;
                bullet->active = false;
                game->enemiesKilled++;
                break;
            }
        }
    }
}

// -------------------------------------------------------------------------

static void handleCollision(Game *game) {
    Entity *player = &game->player;
    
    clampPlayerToWorld(game);

    for (int i = 0; i < MAX_BULLETS; i++) {
        Projectile *bullet = &player->entity_bullets[i];
        if (!bullet->active) continue;

        if (bullet->dim.x + bullet->dim.width < 0.0f ||
            bullet->dim.y + bullet->dim.height < 0.0f ||
            bullet->dim.x > (float)game->world.width ||
            bullet->dim.y > (float)game->world.height) {
            bullet->active = false;
        }
    }
}

static void updateEliteProjectiles(Game *game, Entity *elite) {
    float dt = GetFrameTime();
    Rectangle sector = getSectorRect(game, elite->sectorX, elite->sectorY);

    for (int i = 0; i < MAX_BULLETS; i++) {
        Projectile *beam = &elite->entity_bullets[i];
        if (!beam->active) continue;

        beam->dim.x += beam->projectile_dir.x * beam->projectile_speed * dt;
        beam->dim.y += beam->projectile_dir.y * beam->projectile_speed * dt;

        if (!CheckCollisionRecs(sector, beam->dim)) {
            beam->active = false;
            continue;
        }

        Rectangle playerRec = getEntityRect(&game->player, game->entity_texture.Player, PLAYER_SCALE);
        bool hit = false;

        for (int j = 0; j < MAX_ENEMIES; j++) {
            Entity *enemy = &game->enemies[j];
            if (enemy->lives == 0) continue;
            if (!entityInSector(game, enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE, elite->sectorX, elite->sectorY)) continue;

            Rectangle enemyRec = getEntityRect(enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
            if (CheckCollisionRecs(enemyRec, beam->dim)) {
                enemy->lives = 0;
                game->enemiesKilled++;
                beam->active = false;
                hit = true;
                break;
            }
        }

        if (hit) continue;

        if (CheckCollisionRecs(playerRec, beam->dim)) {
            if (game->playerHitCooldown <= 0.0f) {
                game->player.lives--;
                game->playerHitCooldown = 0.75f;
            }
            beam->active = false;
        }
    }
}

static void updateElites(Game *game) {
    float dt = GetFrameTime();
    Vector2 playerCenter = getEntityCenter(&game->player, game->entity_texture.Player, PLAYER_SCALE);

    for (int i = 0; i < MAX_ELITES; i++) {
        Entity *elite = &game->elites[i];
        if (elite->lives == 0) continue;

        Rectangle eliteRec = getEntityRect(elite, game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE);
        
        for (int j = 0; j < MAX_BULLETS; j++) {
            Projectile *bullet = &game->player.entity_bullets[j];
            if (!bullet->active) continue;

            if (CheckCollisionRecs(eliteRec, bullet->dim)) {
                bullet->active = false;
                elite->lives--;
                if (elite->lives <= 0) {
                    elite->lives = 0;
                    awardEliteDeath(game, i);
                }
                break;
            }
        }

        if (elite->lives == 0) continue;

        elite->entity_shooting_cooldown -= dt;
        
        if (elite->entity_shooting_cooldown <= 0.0f) {
            if (game->world.playerSectorX == elite->sectorX && game->world.playerSectorY == elite->sectorY) {
                Vector2 eliteCenter = getEntityCenter(elite, game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE);
                if (!eliteBeamBlocked(game, elite, eliteCenter, playerCenter)) {
                    fireEliteBeam(game, elite);
                    elite->entity_shooting_cooldown = ELITE_BEAM_COOLDOWN;
                } else {
                    elite->entity_shooting_cooldown = ELITE_BEAM_RETRY;
                }
            } else {
                elite->entity_shooting_cooldown = ELITE_BEAM_RETRY;
            }
        }

        updateEliteProjectiles(game, elite);
    }
}

void runGamePhysics(Game *game) {
    float dt = GetFrameTime();
    game->timer += dt;
    game->elapsedTime += dt;
    if (game->timer > 100.0f) game->timer = 0.0f;

    if (game->playerHitCooldown > 0.0f) {
        game->playerHitCooldown -= dt;
        if (game->playerHitCooldown < 0.0f) game->playerHitCooldown = 0.0f;
    }

    Entity *player = &game->player;

    player->entity_dir = getPlayerDirection();
    player->entity_pos = Vector2Add(player->entity_pos, Vector2Scale(player->entity_dir, player->entity_speed * dt));

    player->entity_shooting_cooldown -= dt;
    if (player->entity_shooting_cooldown <= 0.0f && getShootingDirection(&player->entity_shooting_dir)) {
        PlaySound(game->gamesound.laser_sound);
        makeBullet(game);
        player->entity_shooting_cooldown = 0.25f;
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        Projectile *bullet = &player->entity_bullets[i];
        if (!bullet->active) continue;

        bullet->dim.x += bullet->projectile_dir.x * bullet->projectile_speed * dt;
        bullet->dim.y += bullet->projectile_dir.y * bullet->projectile_speed * dt;
    }

    handleCollision(game);
    clampPlayerToWorld(game);
    updatePlayerSector(game);

    updateEnemies(game);
    updateElites(game);
    updateCamera(game);
}

static void drawBullets(const Projectile *bullets) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            float rot = atan2f(bullets[i].projectile_dir.y, bullets[i].projectile_dir.x) * RAD2DEG + 90.0f;
            DrawRectanglePro(
                bullets[i].dim,
                (Vector2){ bullets[i].dim.width * 0.5f, bullets[i].dim.height * 0.5f },
                rot,
                bullets[i].projectile_color
            );
        }
    }
}

static void drawEntity(const Entity *entity, Texture2D texture, float scale) {
    if (entity->lives == 0) return;
    DrawTextureEx(texture, entity->entity_pos, 0.0f, scale, WHITE);
}

static void drawPlayer(Game *game) {
    Color tint = WHITE;

    if (game->playerHitCooldown > 0.0f) {
        int blink = ((int)(game->elapsedTime * 20.0f)) & 2;
        tint.a = blink ? 255 : 60;
    }

    DrawTextureEx(game->entity_texture.Player, game->player.entity_pos, 0.0f, PLAYER_SCALE, tint);
}

void drawGame(Game *game) {
    drawBackground(game->gameWidth, game->gameHeight, game->timer);

    BeginMode2D(game->camera);
    {
        drawBullets(game->player.entity_bullets);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            drawEntity(&game->enemies[i], game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        }

        for (int i = 0; i < MAX_ELITES; i++) {
            drawBullets(game->elites[i].entity_bullets);
            drawEntity(&game->elites[i], game->entity_texture.Enemy_1, ELITE_ENEMY_SCALE);
        }

        drawPlayer(game);
    }
    EndMode2D();

    drawPanel(game);
}
