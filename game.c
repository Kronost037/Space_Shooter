#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "raylib.h"
#include "raymath.h"


#define SCREEN_WIDTH 1500
#define SCREEN_HEIGHT 900
#define PLAYER_SCALE 0.03f
#define MAX_BULLETS 10

typedef enum {
    STATE_MENU = 0,
    STATE_GAME
} State;

typedef struct S_projectile {
    bool active;
    Vector2 projectile_pos_start;
    Vector2 projectile_size;
    Vector2 projectile_speed;
    Vector2 projectile_dir;
    Color projectile_color;
} Projectile;

typedef struct S_entity {
    Texture2D entity_texture;
    Vector2 entity_pos;
    Vector2 entity_speed;
    Vector2 entity_dir;
    Projectile entity_bullets[MAX_BULLETS];
} Entity;


void initializePlayer(Entity *player) {
    player->entity_texture = LoadTexture("Assets/ufo.png");
    player->entity_pos = (Vector2){0};
    player->entity_speed = (Vector2){600, 600};
    player->entity_dir = (Vector2){0};
}

typedef struct S_menu {
    Music bg_song;
    Rectangle startButton;
    Rectangle exitButton;
    Vector2 mousePos;
    Texture2D stars;
    float timer;
} Menu;

void initializeMenu(Menu *menu) {
    menu->bg_song = LoadMusicStream("Assets/menu.mp3");
    menu->startButton = (Rectangle){ (float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 - 40, 200, 50 };
    menu->exitButton = (Rectangle){ (float)SCREEN_WIDTH/2 - 100, (float)SCREEN_HEIGHT/2 + 30, 200, 50 };
    menu->mousePos = (Vector2){ 0.0f, 0.0f };
    menu->stars = LoadTexture("Assets/star.png");
}

typedef struct S_game {
    bool quit;
    State currentState;
    Menu menu;
    Texture2D sky_texture;
    Entity player;
    Font font;
    float timer;
} Game;

void initializeGame(Game *game) {
    game->quit = false;
    game->currentState = STATE_MENU;

    game->player = (Entity){0};
    initializePlayer(&game->player);
    
    game->menu = (Menu){0};
    initializeMenu(&game->menu);

    game->font = LoadFontEx("Assets/font/KnightWarrior.otf", 40, NULL, 0);
}

Vector2 getShootingDirection() {
    Vector2 entity_dir = {0};
    
    if(IsKeyDown(KEY_D)) {
        entity_dir.x += 1;    
    }
    if(IsKeyDown(KEY_A)) {
        entity_dir.x -= 1;
    }
    if(IsKeyDown(KEY_S)) {
        entity_dir.y += 1;
    }
    if(IsKeyDown(KEY_W)) {
        entity_dir.y -= 1;
    }

    if (entity_dir.x == 0.0f && entity_dir.y == 0.0f) {
        entity_dir.y = -1.0f; 
    }
    
    return Vector2Normalize(entity_dir);
}

void makeBullet(Entity *player, Projectile *bullets) {
    Projectile bullet = {0};
    
    float halfPlayerWidth = ((float)player->entity_texture.width * PLAYER_SCALE) / 2.0f;
    float halfPlayerHeight = ((float)player->entity_texture.height * PLAYER_SCALE) / 2.0f;
    bullet.projectile_pos_start = (Vector2){ player->entity_pos.x + halfPlayerWidth,
                                             player->entity_pos.y + halfPlayerHeight };
    
    bullet.projectile_dir = getShootingDirection();
    
    bullet.projectile_size = (Vector2){ 4.0f, 20.0f }; 
    
    bullet.projectile_speed = (Vector2){ player->entity_speed.x + 800.0f, player->entity_speed.y + 800.0f }; 
    bullet.projectile_color = RED;
    bullet.active = true;
    
    for(int i = 0; i < MAX_BULLETS; i++) {
        if(!bullets[i].active) {
            bullets[i] = bullet;
            break;
        }
    }
}

void drawBullet(Projectile *bullets) {
    for(int i = 0; i < MAX_BULLETS; i++) {
        if(bullets[i].active) {
            Vector2 bullet_end = {
                bullets[i].projectile_pos_start.x - (bullets[i].projectile_dir.x * bullets[i].projectile_size.y),
                bullets[i].projectile_pos_start.y - (bullets[i].projectile_dir.y * bullets[i].projectile_size.y)
            };
            
            DrawLineEx(bullets[i].projectile_pos_start, bullet_end, bullets[i].projectile_size.x, bullets[i].projectile_color);
        }
    }
}

Vector2 getPlayerDirection() {
    Vector2 entity_dir = {0};
    
    if(IsKeyDown(KEY_RIGHT)) {
        entity_dir.x += 1;    
    }
    if(IsKeyDown(KEY_LEFT)) {
        entity_dir.x -= 1;
    }
    if(IsKeyDown(KEY_DOWN)) {
        entity_dir.y += 1;
    }
    if(IsKeyDown(KEY_UP)) {
        entity_dir.y -= 1;
    }

    return Vector2Normalize(entity_dir);
}

void handleCollision(Game *game) {
    Entity *player = &game->player;
    
    float entity_scaledWidth = (float)player->entity_texture.width * PLAYER_SCALE;
    float entity_scaledHeight = (float)player->entity_texture.height * PLAYER_SCALE;

    // Boundary for Player
    if (player->entity_pos.x < 0)
        player->entity_pos.x = 0;
    if (player->entity_pos.x > SCREEN_WIDTH - entity_scaledWidth)
        player->entity_pos.x = SCREEN_WIDTH - entity_scaledWidth;
    if (player->entity_pos.y < 0)
        player->entity_pos.y = 0;
    if (player->entity_pos.y > SCREEN_HEIGHT - entity_scaledHeight)
        player->entity_pos.y = SCREEN_HEIGHT - entity_scaledHeight;


    // Boundary for Bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (player->entity_bullets[i].projectile_pos_start.y < 0 ||
            player->entity_bullets[i].projectile_pos_start.y > SCREEN_HEIGHT ||
            player->entity_bullets[i].projectile_pos_start.x < 0 ||
            player->entity_bullets[i].projectile_pos_start.x > SCREEN_WIDTH) {
            
            player->entity_bullets[i].active = false;
        }
    }
}

void runGamePhysics(Game *game) {
    float delta_time = GetFrameTime();

    game->timer += delta_time;
    if(game->timer > 100.0f) game->timer = 0;

    Entity *player = &game->player;
    
    player->entity_dir = getPlayerDirection();


    // Update Player Position
    player->entity_pos.x += player->entity_dir.x * player->entity_speed.x * delta_time;
    player->entity_pos.y += player->entity_dir.y * player->entity_speed.y * delta_time;


    
    if(IsKeyPressed(KEY_W) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_A)) {
        makeBullet(player, player->entity_bullets);
    }

    // Update Bullet Position
    for(int i = 0; i < MAX_BULLETS; i++) {
        Projectile *bullet = &player->entity_bullets[i];
        if(bullet->active) {
            bullet->projectile_pos_start.x += bullet->projectile_dir.x * bullet->projectile_speed.x * delta_time;
            bullet->projectile_pos_start.y += bullet->projectile_dir.y * bullet->projectile_speed.y * delta_time;
        }
    }

    handleCollision(game);
}

void runMenuPhysics(Game *game) {
    Menu *menu = &game->menu;

    UpdateMusicStream(menu->bg_song);
    menu->timer += GetFrameTime();

    if(menu->timer > 100.0f) menu->timer = 0;
}

void handleGameState(Game *game) {
    Menu *menu = &game->menu;
    menu->mousePos = GetMousePosition();

    switch (game->currentState) {
    case STATE_MENU: {
        if (CheckCollisionPointRec(menu->mousePos, menu->startButton)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                game->currentState = STATE_GAME;
                DisableCursor();
                PauseMusicStream(menu->bg_song);
            }
        }
        
        if (CheckCollisionPointRec(menu->mousePos, menu->exitButton)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                game->quit = true;
            }
        }
    } break;
        
    case STATE_GAME: {
        if (IsKeyPressed(KEY_ESCAPE)) {
            game->currentState = STATE_MENU;
            EnableCursor();
            ResumeMusicStream(menu->bg_song);
        }
    } break;
        
    default : fprintf(stderr, "UNREACHABLE: Not a Valid GAMESTATE.\n");
    }
}

void drawBackground(float timer) {
    DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                           (Color){ 5, 8, 18, 255 },
                           (Color){ 10, 14, 28, 255 });

    DrawRectangle(0, 0, SCREEN_WIDTH, 120, (Color){ 0, 0, 0, 30 });
    DrawRectangle(0, SCREEN_HEIGHT - 120, SCREEN_WIDTH, 120, (Color){ 0, 0, 0, 40 });

    
    for (int i = 0; i < 45; i++) {
        float x = fmodf((i * 187.0f + timer * 10.0f), SCREEN_WIDTH);
        float y = fmodf((i * 97.0f + timer * 4.0f), SCREEN_HEIGHT);
        float r = 1.0f + (i % 3) * 0.4f;
        int a = 70 + (i % 4) * 20;
        DrawCircleV((Vector2){ x, y }, r, (Color){ 220, 230, 255, (unsigned char)a });
    }
}

void drawMenu(Game *game) {
    Menu *menu = &game->menu;

    drawBackground(menu->timer);
    
    Rectangle menuPanel = {
        SCREEN_WIDTH * 0.5f - 275.0f,
        200.0f,
        550.0f,
        420.0f
    };

    float centerX = menuPanel.x + menuPanel.width * 0.5f;

    menu->startButton = (Rectangle){
        centerX - 100.0f,
        menuPanel.y + 205.0f,
        200.0f,
        50.0f
    };

    menu->exitButton = (Rectangle){
        centerX - 100.0f,
        menuPanel.y + 275.0f,
        200.0f,
        50.0f
    };

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

    float fontSize = 78.0f;
    float fontSpacing = 4.0f;
    Vector2 textSize = MeasureTextEx(game->font, "SPACE SHOOTER", fontSize, fontSpacing);
    Vector2 textPos = {
        centerX - textSize.x / 2.0f,
        menuPanel.y + 34.0f
    };

    DrawTextEx(game->font, "SPACE SHOOTER",
               (Vector2){ textPos.x + 2.0f, textPos.y + 3.0f },
               fontSize, fontSpacing, (Color){ 0, 0, 0, 120 });

    DrawTextEx(game->font, "SPACE SHOOTER", textPos, fontSize, fontSpacing,
               (Color){ 225, 225, 240, 255 });

    float lineW = textSize.x + 36.0f;
    float lineX = centerX - lineW / 2.0f;
    DrawRectangleRec((Rectangle){ lineX, textPos.y - 10.0f, lineW, 2.0f }, (Color){ 145, 30, 230, 180 });
    DrawRectangleRec((Rectangle){ lineX + 26.0f, textPos.y + textSize.y + 10.0f, lineW - 52.0f, 1.0f }, (Color){ 60, 200, 255, 95 });

    const char *subtitle = "ARE YOU READY?";
    float subSize = 20.0f;
    float subSpace = 2.0f;
    Vector2 subText = MeasureTextEx(game->font, subtitle, subSize, subSpace);
    Vector2 subPos = {
        centerX - subText.x / 2.0f,
        menuPanel.y + 145.0f
    };

    DrawTextEx(game->font, subtitle,
               (Vector2){ subPos.x + 1.0f, subPos.y + 1.0f },
               subSize, subSpace, (Color){ 0, 0, 0, 110 });
    DrawTextEx(game->font, subtitle, subPos, subSize, subSpace,
               (Color){ 180, 190, 210, 255 });

    bool mouseOverStart = CheckCollisionPointRec(menu->mousePos, menu->startButton);
    bool mouseOverExit  = CheckCollisionPointRec(menu->mousePos, menu->exitButton);

    DrawRectangleRounded(
        (Rectangle){ menu->startButton.x + 4.0f, menu->startButton.y + 6.0f, menu->startButton.width, menu->startButton.height },
        0.22f, 12, (Color){ 0, 0, 0, 85 }
    );
    DrawRectangleRounded(menu->startButton, 0.22f, 12,
                         mouseOverStart ? (Color){ 255, 255, 255, 28 } : (Color){ 255, 255, 255, 16 });
    DrawRectangleLinesEx(menu->startButton, 2.0f,
                         mouseOverStart ? (Color){ 255, 255, 255, 160 } : (Color){ 255, 255, 255, 70 });
    DrawRectangleRec((Rectangle){ menu->startButton.x + 12.0f, menu->startButton.y + 10.0f, 5.0f, menu->startButton.height - 20.0f },
                     (Color){ 145, 30, 230, mouseOverStart ? 220 : 140 });
    DrawRectangleRec((Rectangle){ menu->startButton.x + 3.0f, menu->startButton.y + 3.0f, menu->startButton.width - 6.0f, menu->startButton.height * 0.33f },
                     (Color){ 255, 255, 255, 14 });

    fontSize = 42.0f;
    fontSpacing = 2.0f;
    textSize = MeasureTextEx(game->font, "START", fontSize, fontSpacing);
    textPos = (Vector2){
        menu->startButton.x + menu->startButton.width / 2.0f - textSize.x / 2.0f,
        menu->startButton.y + menu->startButton.height / 2.0f - textSize.y / 2.0f - 2.0f
    };

    DrawTextEx(game->font, "START",
               (Vector2){ textPos.x + 1.5f, textPos.y + 1.5f },
               fontSize, fontSpacing, (Color){ 0, 0, 0, 120 });
    DrawTextEx(game->font, "START", textPos, fontSize, fontSpacing,
               mouseOverStart ? (Color){ 245, 245, 255, 255 } : (Color){ 225, 225, 235, 255 });

    DrawRectangleRounded(
        (Rectangle){ menu->exitButton.x + 4.0f, menu->exitButton.y + 6.0f, menu->exitButton.width, menu->exitButton.height },
        0.22f, 12, (Color){ 0, 0, 0, 85 }
    );
    DrawRectangleRounded(menu->exitButton, 0.22f, 12,
                         mouseOverExit ? (Color){ 255, 255, 255, 26 } : (Color){ 255, 255, 255, 14 });
    DrawRectangleLinesEx(menu->exitButton, 2.0f,
                         mouseOverExit ? (Color){ 255, 255, 255, 150 } : (Color){ 255, 255, 255, 66 });
    DrawRectangleRec((Rectangle){ menu->exitButton.x + 12.0f, menu->exitButton.y + 10.0f, 5.0f, menu->exitButton.height - 20.0f },
                     (Color){ 60, 200, 255, mouseOverExit ? 220 : 140 });
    DrawRectangleRec((Rectangle){ menu->exitButton.x + 3.0f, menu->exitButton.y + 3.0f, menu->exitButton.width - 6.0f, menu->exitButton.height * 0.33f },
                     (Color){ 255, 255, 255, 12 });

    textSize = MeasureTextEx(game->font, "EXIT", fontSize, fontSpacing);
    textPos = (Vector2){
        menu->exitButton.x + menu->exitButton.width / 2.0f - textSize.x / 2.0f,
        menu->exitButton.y + menu->exitButton.height / 2.0f - textSize.y / 2.0f - 2.0f
    };

    DrawTextEx(game->font, "EXIT",
               (Vector2){ textPos.x + 1.5f, textPos.y + 1.5f },
               fontSize, fontSpacing, (Color){ 0, 0, 0, 120 });
    DrawTextEx(game->font, "EXIT", textPos, fontSize, fontSpacing,
               mouseOverExit ? (Color){ 245, 245, 255, 255 } : (Color){ 225, 225, 235, 255 });

    DrawRectangleRec((Rectangle){ menuPanel.x + 20.0f, menuPanel.y + menuPanel.height - 34.0f, menuPanel.width - 40.0f, 1.0f },
                     (Color){ 255, 255, 255, 22 });
}

void drawGame(Game *game) {
    drawBackground(game->timer);
    DrawTextureEx(game->player.entity_texture, game->player.entity_pos, 0.0f, PLAYER_SCALE, WHITE);
    drawBullet(game->player.entity_bullets);
    
}

void unload(Game *game) {
    UnloadMusicStream(game->menu.bg_song);
    UnloadFont(game->font);
    UnloadTexture(game->menu.stars);
    UnloadTexture(game->player.entity_texture);
}

int main() {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetExitKey(KEY_NULL);

    InitAudioDevice();
    
    Game game = {0};
    initializeGame(&game);

    PlayMusicStream(game.menu.bg_song);
    SetTargetFPS(60);
    while(!WindowShouldClose() && !game.quit) {

        handleGameState(&game);

        if (game.currentState == STATE_GAME) runGamePhysics(&game);
        else if (game.currentState == STATE_MENU) runMenuPhysics(&game);
        
        BeginDrawing();
        {
            ClearBackground(BLACK);

            if (game.currentState == STATE_GAME) drawGame(&game);
            else if (game.currentState == STATE_MENU) drawMenu(&game);
       
        }
        EndDrawing();
    }

    unload(&game);
    CloseAudioDevice();
    CloseWindow();
    
	return 0;
}
