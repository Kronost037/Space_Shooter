#include "game.h"
#include "leaderboard.h"
#include "menu.h"
#include "panel.h"
#include "background.h"

#include "raymath.h"
#include <math.h>
#include <raylib.h>

static void initializeSound(GameSound *gamesound) {
    gamesound->laser_sound = LoadSound("src/Assets/laser_shot.mp3");
    SetSoundVolume(gamesound->laser_sound, 0.1f);
}

static Vector2 getScaledTextureSize(Texture2D texture, float scale) {
    return (Vector2){
        (float)texture.width * scale,
        (float)texture.height * scale
    };
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

void initializeGame(Game *game) {
    *game = (Game){0};
    game->quit = false;
    game->showPanel = true;
    game->currentState = STATE_MENU;
    game->timer = 0.0f;
    game->enemiesKilled = 0;

    game->player.lives = MAX_LIVES;
    game->playerName[0] = '\0';
    game->scoreSubmitted = false;
    game->playerHitCooldown = 0.0f;
    
    game->entity_texture.Player = LoadTexture("src/Assets/ufo.png");
    game->entity_texture.Enemy_1 = LoadTexture("src/Assets/enemy_1.png");
    
    refreshLayout(game);

    initializePlayer(&game->player);

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

static void spawnEnemy(Game *game) {

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (enemy->lives > 0) continue;

        *enemy = (Entity){0};
        enemy->lives = 1;
        enemy->entity_speed = (float)GetRandomValue(150, 250);
       
        Vector2 enemySize = getScaledTextureSize(game->entity_texture.Enemy_1, ENEMY_1_SCALE);
        int edge = GetRandomValue(0, 3);

        switch (edge) {
            case 0: // top
                enemy->entity_pos.x = (float)GetRandomValue(0, (int)fmaxf(0.0f, (float)game->gameWidth - enemySize.x));
                enemy->entity_pos.y = -enemySize.y;
                break;
            case 1: // right
                enemy->entity_pos.x = (float)game->gameWidth + enemySize.x;
                enemy->entity_pos.y = (float)GetRandomValue(0, (int)fmaxf(0.0f, (float)game->gameHeight - enemySize.y));
                break;
            case 2: // bottom
                enemy->entity_pos.x = (float)GetRandomValue(0, (int)fmaxf(0.0f, (float)game->gameWidth - enemySize.x));
                enemy->entity_pos.y = (float)game->gameHeight + enemySize.y;
                break;
            case 3: // left
                enemy->entity_pos.x = -enemySize.x;
                enemy->entity_pos.y = (float)GetRandomValue(0, (int)fmaxf(0.0f, (float)game->gameHeight - enemySize.y));
                break;
        }

        break;
    }
}

static void updateEnemies(Game *game) {
    float dt = GetFrameTime();

    static float spawnTimer = 0.0f;
    spawnTimer += dt;
    if (spawnTimer >= 1.0f) {
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
    Vector2 playerSize = getScaledTextureSize(game->entity_texture.Player, PLAYER_SCALE);

    if (player->entity_pos.x < 0.0f) player->entity_pos.x = 0.0f;
    if (player->entity_pos.y < 0.0f) player->entity_pos.y = 0.0f;
    if (player->entity_pos.x > (float)game->gameWidth - playerSize.x) player->entity_pos.x = (float)game->gameWidth - playerSize.x;
    if (player->entity_pos.y > (float)game->gameHeight - playerSize.y) player->entity_pos.y = (float)game->gameHeight - playerSize.y;

    for (int i = 0; i < MAX_BULLETS; i++) {
        Projectile *bullet = &player->entity_bullets[i];
        if (!bullet->active) continue;

        if (bullet->dim.x + bullet->dim.width < 0.0f ||
            bullet->dim.y + bullet->dim.height < 0.0f ||
            bullet->dim.x > (float)game->gameWidth ||
            bullet->dim.y > (float)game->gameHeight) {
            bullet->active = false;
        }
    }
}

void runGamePhysics(Game *game) {
    float dt = GetFrameTime();
    game->timer += dt;
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
}

static void drawEntity(const Entity *entity, Texture2D texture, Color tint, float scale) {
    if (entity->lives == 0) return;
    DrawTextureEx(texture, entity->entity_pos, 0.0f, scale, tint);
}

void drawGame(Game *game) {
    drawBackground(game->gameWidth, game->gameHeight, game->timer);

    drawBullet(game->player.entity_bullets);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        drawEntity(&game->enemies[i], game->entity_texture.Enemy_1, WHITE, ENEMY_1_SCALE);
    }

    drawEntity(&game->player, game->entity_texture.Player, WHITE, PLAYER_SCALE);
    
    drawPanel(game);
}
