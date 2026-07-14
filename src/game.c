#include "game.h"
#include "leaderboard.h"
#include "menu.h"
#include "panel.h"
#include "background.h"

#include "raymath.h"
#include <math.h>
#include <raylib.h>


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

static void initializePlayer(Entity *player) {
    *player = (Entity){0};
    player->lives = MAX_LIVES;
    player->entity_pos = (Vector2){ 40.0f, 100.0f };
    player->entity_speed = 600.0f;
    player->entity_dir = (Vector2){0};
    player->entity_shooting_dir = (Vector2){0, -1};
    player->entity_shooting_cooldown = 0.0f;
    player->entity_color = WHITE;
}


static void syncCameraLayout(Game *game) {
    game->camera.offset = (Vector2){
        (float)game->gameWidth * 0.5f,
        (float)game->gameHeight * 0.5f
    };
}

static void initializeWorld(Game *game) {
    game->world.cols = WORLD_COLS;
    game->world.rows = WORLD_ROWS;
    game->world.sectorWidth = WORLD_SECTOR_WIDTH;
    game->world.sectorHeight = WORLD_SECTOR_HEIGHT;
    game->world.width = WORLD_COLS * WORLD_SECTOR_WIDTH;
    game->world.height = WORLD_ROWS * WORLD_SECTOR_HEIGHT;
    game->world.playerSectorX = 0;
    game->world.playerSectorY = 0;
}

static Rectangle getSectorRect(const Game *game, int sx, int sy) {
    return (Rectangle){
        (float)(sx * game->world.sectorWidth),
        (float)(sy * game->world.sectorHeight),
        (float)game->world.sectorWidth,
        (float)game->world.sectorHeight
    };
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

            if (sx != game->world.playerSectorX || sy != game->world.playerSectorY) {
                break;
            }
        }

        Vector2 enemySize = getScaledTextureSize(game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        Rectangle sector = getSectorRect(game, sx, sy);
        Vector2 pos = getRandomPointInsideRect(sector, enemySize);

        *enemy = (Entity){0};
        enemy->lives = 1;
        enemy->entity_speed = (float)GetRandomValue(250, 350);
        enemy->entity_pos = pos;
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

    game->player.entity_dir = (Vector2){ 0.0f, 0.0f };
    game->player.entity_shooting_dir = (Vector2){ 0.0f, -1.0f };
    game->player.entity_shooting_cooldown = 0.0f;

    memset(game->enemies, 0, sizeof(game->enemies));
    memset(game->player.entity_bullets, 0, sizeof(game->player.entity_bullets));

    refreshLayout(game);
    spawnPlayerInRandomSector(game);

    game->camera.target = getEntityCenter(&game->player, game->entity_texture.Player, PLAYER_SCALE);

    DisableCursor();
    PauseMusicStream(game->menu->bg_song);
}

static void initializeSound(GameSound *gamesound) {
    gamesound->laser_sound = LoadSound("src/Assets/laser_shot.mp3");
    SetSoundVolume(gamesound->laser_sound, 0.1f);
}

static void drawBullet(Projectile *bullets) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            float rot = atan2f(bullets[i].projectile_dir.y, bullets[i].projectile_dir.x) * RAD2DEG + 90.0f;
            DrawRectanglePro(
                bullets[i].dim,
                (Vector2){ bullets[i].dim.width / 2.0f, bullets[i].dim.height / 2.0f },
                rot,
                bullets[i].projectile_color
            );
        }
    }
}

void initializeGame(Game *game) {
    *game = (Game){0};
    game->quit = false;
    game->showPanel = true;
    game->currentState = STATE_MENU;
    game->timer = 0.0f;
    game->elapsedTime = 0.0f;
    game->enemiesKilled = 0;

    game->player.lives = MAX_LIVES;
    game->playerName[0] = '\0';
    game->scoreSubmitted = false;
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

    for (int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i] = (Entity){0};
    }

    initializeSound(&game->gamesound);

    game->font = LoadFontEx("src/Assets/font/KnightWarrior.otf", 40, NULL, 0);

    game->menu->startButton = (Button){ .bounds = {0}, .label = "START" };
    game->menu->exitButton = (Button){ .bounds = {0}, .label = "EXIT" };
    
    leaderboardInit(&game->leaderboard);
    leaderboardLoad(&game->leaderboard);
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
            bullet->projectile_dir = player->entity_shooting_dir;
            bullet->projectile_speed = 800.0f;
            bullet->projectile_color = RED;
            bullet->active = true;
            break;
        }
    }
}


static void updateEnemies(Game *game) {
    float dt = GetFrameTime();

    static float spawnTimer = 0.0f;
    spawnTimer += dt;
    if (spawnTimer >= 0.5f) {
        spawnEnemy(game);
        spawnTimer = 0.0f;
    }

    Entity *player = &game->player;
    Vector2 playerCenter = getEntityCenter(player, game->entity_texture.Player, PLAYER_SCALE);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (enemy->lives == 0) continue;

        Vector2 enemyCenter = getEntityCenter(enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        Vector2 dir = Vector2Subtract(playerCenter, enemyCenter);

        if (Vector2LengthSqr(dir) > 0.0001f) {
            dir = Vector2Normalize(dir);
            enemy->entity_pos = Vector2Add(enemy->entity_pos, Vector2Scale(dir, enemy->entity_speed * dt));
        }

        if (enemyVisible(game, enemy)) {
                enemy->last_seen = game->elapsedTime;
            }

        Rectangle enemyRec = getEntityRect(enemy, game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        Rectangle playerRec = getEntityRect(player, game->entity_texture.Player, PLAYER_SCALE);

        if (CheckCollisionRecs(enemyRec, playerRec)) {
            enemy->lives--;
            game->player.lives--;
            continue;
        }

        for (int j = 0; j < MAX_BULLETS; j++) {
            Projectile *bullet = &player->entity_bullets[j];
            if (!bullet->active) continue;

            if (CheckCollisionRecs(enemyRec, bullet->dim)) {
                enemy->lives--;
                bullet->active = false;
                game->enemiesKilled++;
                break;
            }
        }
    }
}

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

void runGamePhysics(Game *game) {
    float dt = GetFrameTime();
    game->timer += dt;
    game->elapsedTime += dt;
    if (game->timer > 100.0f) game->timer = 0.0f;

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
    updateEnemies(game);
    updateCamera(game);
}



static void drawEntity(const Entity *entity, Texture2D texture, Color tint, float scale) {
    if (entity->lives == 0) return;
    DrawTextureEx(texture, entity->entity_pos, 0.0f, scale, tint);
}

void drawGame(Game *game) {
    drawBackground(game->gameWidth, game->gameHeight, game->timer);

    BeginMode2D(game->camera);
    {
        drawBullet(game->player.entity_bullets);

        for (int i = 0; i < MAX_ENEMIES; i++) {
            drawEntity(&game->enemies[i], game->entity_texture.Enemy_1, WHITE, ENEMY_1_SCALE);
        }

        drawEntity(&game->player, game->entity_texture.Player, WHITE, PLAYER_SCALE);
    }
    EndMode2D();

    drawPanel(game);
}
