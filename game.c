
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"

#define SCREEN_WIDTH 1500
#define SCREEN_HEIGHT 900
#define PANEL_WIDTH 320
#define PLAYER_SCALE 0.03f
#define MAX_BULLETS 40
#define MAX_ENEMIES 20

typedef enum {
    STATE_MENU = 0,
    STATE_GAME
} State;

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
    Vector2 entity_pos;     // top-left position
    float entity_speed;
    Vector2 entity_dir;
    Projectile entity_bullets[MAX_BULLETS];
    Vector2 entity_shooting_dir;
    float entity_shooting_cooldown;
    Color entity_color;
} Entity;

typedef struct S_button {
    Rectangle bounds;
    const char *label;
} Button;

typedef struct S_menu {
    Music bg_song;
    Vector2 mousePos;
    Button startButton;
    Button exitButton;
    float timer;
} Menu;

typedef struct S_game {
    bool quit;
    bool showPanel;
    int gameWidth;
    int gameHeight;
    State currentState;
    Menu menu;
    Entity player;
    Entity enemies[MAX_ENEMIES];
    Font font;
    float timer;
    int enemiesKilled;
} Game;

static void refreshLayout(Game *game) {
    game->gameWidth = SCREEN_WIDTH - (game->showPanel ? PANEL_WIDTH : 0);
    game->gameHeight = SCREEN_HEIGHT;
}

static void updateMenuLayout(Game *game) {
    Rectangle menuPanel = {
        SCREEN_WIDTH * 0.5f - 275.0f,
        200.0f,
        550.0f,
        420.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    game->menu.startButton.bounds = (Rectangle){
        centerX - 100.0f,
        menuPanel.y + 205.0f,
        200.0f,
        50.0f
    };

    game->menu.exitButton.bounds = (Rectangle){
        centerX - 100.0f,
        menuPanel.y + 275.0f,
        200.0f,
        50.0f
    };
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
    player->entity_texture = LoadTexture("Assets/ufo.png");
    player->entity_pos = (Vector2){ 40.0f, 100.0f };
    player->entity_speed = 600.0f;
    player->entity_dir = (Vector2){0};
    player->entity_shooting_dir = (Vector2){0, -1};
    player->entity_shooting_cooldown = 0.0f;
    player->entity_color = WHITE;
}

static void initializeMenu(Menu *menu) {
    *menu = (Menu){0};
    menu->bg_song = LoadMusicStream("Assets/menu.mp3");
    menu->mousePos = (Vector2){0.0f, 0.0f};
    menu->timer = 0.0f;
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
    initializeMenu(&game->menu);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i] = (Entity){0};
    }

    game->font = LoadFontEx("Assets/font/KnightWarrior.otf", 40, NULL, 0);

    game->menu.startButton = (Button){ .bounds = {0}, .label = "START" };
    game->menu.exitButton = (Button){ .bounds = {0}, .label = "EXIT" };
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

static void runMenuPhysics(Game *game) {
    UpdateMusicStream(game->menu.bg_song);
    game->menu.timer += GetFrameTime();
    if (game->menu.timer > 100.0f) game->menu.timer = 0.0f;
}

static void drawBackground(int width, int height, float timer) {
    DrawRectangleGradientV(0, 0, width, height,
                           (Color){ 5, 8, 18, 255 },
                           (Color){ 10, 14, 28, 255 });

    DrawRectangle(0, 0, width, 120, (Color){ 0, 0, 0, 30 });
    DrawRectangle(0, height - 120, width, 120, (Color){ 0, 0, 0, 40 });

    for (int i = 0; i < 45; i++) {
        float x = fmodf((i * 187.0f + timer * 10.0f), (float)width);
        float y = fmodf((i * 97.0f + timer * 4.0f), (float)height);
        float r = 1.0f + (i % 3) * 0.4f;
        int a = 70 + (i % 4) * 20;
        DrawCircleV((Vector2){ x, y }, r, (Color){ 220, 230, 255, (unsigned char)a });
    }
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

    DrawRectangleRec((Rectangle){ panel.x + 16.0f, 18.0f, panel.width - 32.0f, 2.0f }, (Color){ 60, 200, 255, 110 });
    DrawRectangleRec((Rectangle){ panel.x + 16.0f, 22.0f, panel.width - 70.0f, 1.0f }, (Color){ 145, 30, 230, 90 });

    DrawTextEx(game->font, "HUD", (Vector2){ panel.x + 18.0f, 36.0f }, 26.0f, 2.0f, (Color){ 230, 235, 245, 255 });

    char buf[128];
    snprintf(buf, sizeof(buf), "Enemies Killed: %d", game->enemiesKilled);
    DrawTextEx(game->font, buf, (Vector2){ panel.x + 18.0f, 92.0f }, 22.0f, 1.5f, (Color){ 220, 220, 235, 255 });

    snprintf(buf, sizeof(buf), "Playfield: %d x %d", game->gameWidth, game->gameHeight);
    DrawTextEx(game->font, buf, (Vector2){ panel.x + 18.0f, 128.0f }, 18.0f, 1.0f, (Color){ 180, 190, 210, 255 });

    DrawTextEx(game->font, "M: toggle panel", (Vector2){ panel.x + 18.0f, 186.0f }, 18.0f, 1.0f, (Color){ 180, 190, 210, 255 });
    DrawTextEx(game->font, "WASD: shoot", (Vector2){ panel.x + 18.0f, 214.0f }, 18.0f, 1.0f, (Color){ 180, 190, 210, 255 });
    DrawTextEx(game->font, "Arrow keys: move", (Vector2){ panel.x + 18.0f, 242.0f }, 18.0f, 1.0f, (Color){ 180, 190, 210, 255 });
}

static void drawButton(Game *game, const Button *button, bool hovered, Color accent) {
    Rectangle r = button->bounds;

    DrawRectangleRounded((Rectangle){ r.x + 4.0f, r.y + 6.0f, r.width, r.height }, 0.22f, 12, (Color){ 0, 0, 0, 85 });
    DrawRectangleRounded(r, 0.22f, 12, hovered ? (Color){ 255, 255, 255, 28 } : (Color){ 255, 255, 255, 16 });
    DrawRectangleLinesEx(r, 2.0f, hovered ? (Color){ 255, 255, 255, 160 } : (Color){ 255, 255, 255, 70 });

    DrawRectangleRec((Rectangle){ r.x + 12.0f, r.y + 10.0f, 5.0f, r.height - 20.0f }, accent);
    DrawRectangleRec((Rectangle){ r.x + 3.0f, r.y + 3.0f, r.width - 6.0f, r.height * 0.33f }, (Color){ 255, 255, 255, 14 });

    float fontSize = 42.0f;
    float fontSpacing = 2.0f;
    Vector2 textSize = MeasureTextEx(game->font, button->label, fontSize, fontSpacing);
    Vector2 textPos = {
        r.x + r.width / 2.0f - textSize.x / 2.0f,
        r.y + r.height / 2.0f - textSize.y / 2.0f - 2.0f
    };

    DrawTextEx(game->font, button->label,
               (Vector2){ textPos.x + 1.5f, textPos.y + 1.5f },
               fontSize, fontSpacing, (Color){ 0, 0, 0, 120 });
    DrawTextEx(game->font, button->label, textPos, fontSize, fontSpacing,
               hovered ? (Color){ 245, 245, 255, 255 } : (Color){ 225, 225, 235, 255 });
}

static void drawMenu(Game *game) {
    drawBackground(SCREEN_WIDTH, SCREEN_HEIGHT, game->menu.timer);

    Rectangle menuPanel = {
        SCREEN_WIDTH * 0.5f - 275.0f,
        200.0f,
        550.0f,
        420.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    updateMenuLayout(game);

    DrawRectangleRec((Rectangle){ 0, menuPanel.y + 30.0f, SCREEN_WIDTH, 2 }, (Color){ 145, 30, 230, 28 });
    DrawRectangleRec((Rectangle){ 0, menuPanel.y + 33.0f, SCREEN_WIDTH, 1 }, (Color){ 60, 200, 255, 18 });

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x - 6.0f, menuPanel.y - 6.0f, menuPanel.width + 12.0f, menuPanel.height + 12.0f },
        0.08f, 16, (Color){ 145, 30, 230, 18 }
    );

    DrawRectangleRounded(menuPanel, 0.08f, 16, (Color){ 12, 16, 30, 210 });

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 2.0f, menuPanel.width - 4.0f, menuPanel.height * 0.34f },
        0.08f, 16, (Color){ 255, 255, 255, 12 }
    );

    DrawRectangleRounded(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + menuPanel.height * 0.58f, menuPanel.width - 4.0f, menuPanel.height * 0.40f },
        0.08f, 16, (Color){ 0, 0, 0, 28 }
    );

    DrawRectangleLinesEx(menuPanel, 2.0f, (Color){ 255, 255, 255, 48 });
    DrawRectangleLinesEx(
        (Rectangle){ menuPanel.x + 2.0f, menuPanel.y + 2.0f, menuPanel.width - 4.0f, menuPanel.height - 4.0f },
        1.0f, (Color){ 145, 30, 230, 34 }
    );

    DrawRectangleRec((Rectangle){ menuPanel.x + 18.0f, menuPanel.y + 18.0f, menuPanel.width - 36.0f, 2.0f },
                     (Color){ 60, 200, 255, 120 });
    DrawRectangleRec((Rectangle){ menuPanel.x + 18.0f, menuPanel.y + 22.0f, menuPanel.width - 120.0f, 1.0f },
                     (Color){ 145, 30, 230, 90 });

    const char *title = "SPACE SHOOTER";
    float fontSize = 78.0f;
    float fontSpacing = 4.0f;
    Vector2 textSize = MeasureTextEx(game->font, title, fontSize, fontSpacing);
    Vector2 textPos = { centerX - textSize.x / 2.0f, menuPanel.y + 34.0f };

    DrawTextEx(game->font, title,
               (Vector2){ textPos.x + 2.0f, textPos.y + 3.0f },
               fontSize, fontSpacing, (Color){ 0, 0, 0, 120 });
    DrawTextEx(game->font, title, textPos, fontSize, fontSpacing, (Color){ 225, 225, 240, 255 });

    float lineW = textSize.x + 36.0f;
    float lineX = centerX - lineW / 2.0f;
    DrawRectangleRec((Rectangle){ lineX, textPos.y - 10.0f, lineW, 2.0f }, (Color){ 145, 30, 230, 180 });
    DrawRectangleRec((Rectangle){ lineX + 26.0f, textPos.y + textSize.y + 10.0f, lineW - 52.0f, 1.0f }, (Color){ 60, 200, 255, 95 });

    const char *subtitle = "ARE YOU READY?";
    float subSize = 20.0f;
    float subSpace = 2.0f;
    Vector2 subText = MeasureTextEx(game->font, subtitle, subSize, subSpace);
    Vector2 subPos = { centerX - subText.x / 2.0f, menuPanel.y + 145.0f };

    DrawTextEx(game->font, subtitle,
               (Vector2){ subPos.x + 1.0f, subPos.y + 1.0f },
               subSize, subSpace, (Color){ 0, 0, 0, 110 });
    DrawTextEx(game->font, subtitle, subPos, subSize, subSpace, (Color){ 180, 190, 210, 255 });

    bool mouseOverStart = CheckCollisionPointRec(game->menu.mousePos, game->menu.startButton.bounds);
    bool mouseOverExit = CheckCollisionPointRec(game->menu.mousePos, game->menu.exitButton.bounds);

    drawButton(game, &game->menu.startButton, mouseOverStart, (Color){ 145, 30, 230, mouseOverStart ? 220 : 140 });
    drawButton(game, &game->menu.exitButton, mouseOverExit, (Color){ 60, 200, 255, mouseOverExit ? 220 : 140 });

    DrawRectangleRec((Rectangle){ menuPanel.x + 20.0f, menuPanel.y + menuPanel.height - 34.0f, menuPanel.width - 40.0f, 1.0f },
                     (Color){ 255, 255, 255, 22 });
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
    game->menu.mousePos = GetMousePosition();

    switch (game->currentState) {
        case STATE_MENU:
            updateMenuLayout(game);

            if (CheckCollisionPointRec(game->menu.mousePos, game->menu.startButton.bounds) &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                game->currentState = STATE_GAME;
                DisableCursor();
                PauseMusicStream(game->menu.bg_song);
            }

            if (CheckCollisionPointRec(game->menu.mousePos, game->menu.exitButton.bounds) &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                game->quit = true;
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
                ResumeMusicStream(game->menu.bg_song);
            }
            break;

        default:
            fprintf(stderr, "UNREACHABLE: Not a valid game state.\n");
            break;
    }
}

static void unload(Game *game) {
    UnloadMusicStream(game->menu.bg_song);
    UnloadFont(game->font);
    UnloadTexture(game->player.entity_texture);
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetExitKey(KEY_NULL);

    InitAudioDevice();

    Game game = {0};
    initializeGame(&game);

    PlayMusicStream(game.menu.bg_song);
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
