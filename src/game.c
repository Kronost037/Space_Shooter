#include "game.h"
#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"

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

static Vector2 getScaledTextureSize(Texture2D texture) {
    return (Vector2){
        (float)texture.width * PLAYER_SCALE,
        (float)texture.height * PLAYER_SCALE
    };
}

static Rectangle getEntityRect(const Entity *entity) {
    Vector2 scaled = getScaledTextureSize(entity->entity_texture);
    return (Rectangle){
        entity->entity_pos.x,
        entity->entity_pos.y,
        scaled.x,
        scaled.y
    };
}

static Vector2 getEntityCenter(const Entity *entity) {
    Rectangle r = getEntityRect(entity);
    return (Vector2){ r.x + r.width * 0.5f, r.y + r.height * 0.5f };
}

static void initializePlayer(Entity *player) {
    *player = (Entity){0};
    player->active = true;
    player->entity_texture = LoadTexture("src/Assets/ufo.png");
    player->entity_pos = (Vector2){ 40.0f, 100.0f };
    player->entity_speed = 600.0f;
    player->entity_dir = (Vector2){0};
    player->entity_shooting_dir = (Vector2){0, -1};
    player->entity_shooting_cooldown = 0.0f;
    player->entity_color = WHITE;
}

static void initializeGame(Game *game) {
    *game = (Game){0};
    game->quit = false;
    game->showPanel = true;
    game->currentState = STATE_MENU;
    game->timer = 0.0f;
    game->enemiesKilled = 0;

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

static void makeBullet(Entity *player) {
    Vector2 playerSize = getScaledTextureSize(player->entity_texture);

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
    Entity *player = &game->player;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (enemy->active) continue;

        *enemy = (Entity){0};
        enemy->active = true;
        enemy->entity_texture = player->entity_texture;
        enemy->entity_speed = (float)GetRandomValue(150, 250);
        enemy->entity_color = GREEN;

        Vector2 enemySize = getScaledTextureSize(enemy->entity_texture);
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
    Vector2 playerCenter = getEntityCenter(player);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Entity *enemy = &game->enemies[i];
        if (!enemy->active) continue;

        Vector2 enemyCenter = getEntityCenter(enemy);
        Vector2 dir = Vector2Subtract(playerCenter, enemyCenter);
        if (Vector2LengthSqr(dir) > 0.0001f) {
            dir = Vector2Normalize(dir);
            enemy->entity_pos = Vector2Add(enemy->entity_pos, Vector2Scale(dir, enemy->entity_speed * dt));
        }

        Rectangle enemyRec = getEntityRect(enemy);
        Rectangle playerRec = getEntityRect(player);

        if (CheckCollisionRecs(enemyRec, playerRec)) {
            enemy->active = false;
            continue;
        }

        for (int j = 0; j < MAX_BULLETS; j++) {
            Projectile *bullet = &player->entity_bullets[j];
            if (!bullet->active) continue;

            if (CheckCollisionRecs(enemyRec, bullet->dim)) {
                enemy->active = false;
                bullet->active = false;
                game->enemiesKilled++;
                break;
            }
        }
    }
}

static void handleCollision(Game *game) {
    Entity *player = &game->player;
    Vector2 playerSize = getScaledTextureSize(player->entity_texture);

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

static void runGamePhysics(Game *game) {
    float dt = GetFrameTime();
    game->timer += dt;
    if (game->timer > 100.0f) game->timer = 0.0f;

    Entity *player = &game->player;

    player->entity_dir = getPlayerDirection();
    player->entity_pos = Vector2Add(player->entity_pos, Vector2Scale(player->entity_dir, player->entity_speed * dt));

    player->entity_shooting_cooldown -= dt;
    if (player->entity_shooting_cooldown <= 0.0f && getShootingDirection(&player->entity_shooting_dir)) {
        PlaySound(game->gamesound.laser_sound);
        makeBullet(player);
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

static void drawEntity(const Entity *entity, Color tint) {
    if (!entity->active) return;
    DrawTextureEx(entity->entity_texture, entity->entity_pos, 0.0f, PLAYER_SCALE, tint);
}

static void drawPanel(Game *game) {
    if (!game->showPanel) return;

    Rectangle panel = {
        (float)game->gameWidth,
        0.0f,
        (float)PANEL_WIDTH,
        (float)game->gameHeight
    };

    DrawRectangleRec(panel, (Color){ 12, 16, 30, 255 });
    DrawRectangleLinesEx(panel, 2.0f, (Color){ 255, 255, 255, 40 });
    
    DrawTextEx(game->font, "Space Shooter", (Vector2){ panel.x + 22.0f, 20.0f }, 30.0f, 2.0f, (Color){ 230, 235, 245, 255 });

    
    DrawRectangleRec((Rectangle){ panel.x + 16.0f, 60.0f, panel.width - 32.0f, 2.0f }, (Color){ 60, 200, 255, 110 });
    DrawRectangleRec((Rectangle){ panel.x + 16.0f, 67.0f, panel.width - 70.0f, 1.0f }, (Color){ 145, 30, 230, 90 });

    char buf[128];
    snprintf(buf, sizeof(buf), "Enemies Killed: %d", game->enemiesKilled);
    DrawTextEx(game->font, buf, (Vector2){ panel.x + 18.0f, 76.0f }, 25.0f, 1.5f, (Color){ 220, 220, 235, 255 });

    DrawTextEx(game->font, "Press M to toggle panel", (Vector2){ panel.x + 18.0f, panel.height - 152.0f }, 20.0f, 2.0f, (Color){ 180, 190, 210, 255 });
    DrawTextEx(game->font, "WASD to shoot", (Vector2){ panel.x + 18.0f, panel.height - 126.0f }, 20.0f, 2.0f, (Color){ 180, 190, 210, 255 });
    DrawTextEx(game->font, "Arrow keys to move", (Vector2){ panel.x + 18.0f, panel.height - 100.0f }, 20.0f, 2.0f, (Color){ 180, 190, 210, 255 });
}

static void drawGame(Game *game) {
    drawBackground(game->gameWidth, game->gameHeight, game->timer);

    drawBullet(game->player.entity_bullets);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        drawEntity(&game->enemies[i], RED);
    }

    drawEntity(&game->player, WHITE);
    drawPanel(game);
}

static void handleGameState(Game *game) {
    game->menu->mousePos = GetMousePosition();

    switch (game->currentState) {
    case STATE_MENU:
        if (menuActionReady(game->menu)) {
            MenuAction action = menuConsumeAction(game->menu);
            
            if (action == MENU_ACTION_START) {
                game->currentState = STATE_GAME;
                DisableCursor();
                PauseMusicStream(game->menu->bg_song);
            } else if (action == MENU_ACTION_LEADERBOARD) {
                /* leaderboard logic later */
            } else if (action == MENU_ACTION_EXIT) {
                game->quit = true;
            }
        }
      break;

    case STATE_GAME:
      if (IsKeyPressed(KEY_M)) {
        game->showPanel = !game->showPanel;
        refreshLayout(game);
      }

      if (IsKeyPressed(KEY_ESCAPE)) {
        game->currentState = STATE_MENU;
        EnableCursor();
        ResumeMusicStream(game->menu->bg_song);
      }
      break;

    default:
      fprintf(stderr, "UNREACHABLE: Not a valid game state.\n");
      break;
    }
}

static void unload(Game *game) {
    UnloadMusicStream(game->menu->bg_song);
    UnloadSound(game->menu->hover_sfx);
    UnloadSound(game->menu->click_sfx);
    UnloadFont(game->font);
    UnloadTexture(game->player.entity_texture);
    free(game->menu);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetExitKey(KEY_NULL);

    InitAudioDevice();

    Game game = {0};
    initializeGame(&game);

    PlayMusicStream(game.menu->bg_song);
    SetTargetFPS(60);

    while (!WindowShouldClose() && !game.quit) {
        handleGameState(&game);

        if (game.currentState == STATE_GAME) {
            runGamePhysics(&game);
        } else if (game.currentState == STATE_MENU) {
            runMenuPhysics(&game);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (game.currentState == STATE_GAME) {
            drawGame(&game);
        } else if (game.currentState == STATE_MENU) {
            drawMenu(&game);
        }

        EndDrawing();
    }

    unload(&game);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
