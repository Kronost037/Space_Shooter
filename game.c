#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>


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
} Game;

void initializeGame(Game *game) {
    game->quit = false;
    game->currentState = STATE_MENU;

    game->player = (Entity){0};
    initializePlayer(&game->player);
    
    game->menu = (Menu){0};
    initializeMenu(&game->menu);

    game->sky_texture = LoadTexture("Assets/sky.png");
    game->font = LoadFontEx("Assets/font/KnightWarrior.otf", 40, NULL, 0);
}

void makeBullet(Entity *player, Projectile *bullets) {
    Projectile bullet = {0};
    
    float halfPlayerWidth = ((float)player->entity_texture.width * PLAYER_SCALE) / 2.0f;
    float halfPlayerHeight = ((float)player->entity_texture.height * PLAYER_SCALE) / 2.0f;
    bullet.projectile_pos_start = (Vector2){ player->entity_pos.x + halfPlayerWidth,
                                             player->entity_pos.y + halfPlayerHeight };
    
    bullet.projectile_dir = player->entity_dir;
    if(bullet.projectile_dir.x == 0 && bullet.projectile_dir.y == 0) {
        bullet.projectile_dir.y = -1;
    } else {
        bullet.projectile_dir = Vector2Normalize(bullet.projectile_dir);
    }

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

    Entity *player = &game->player;
    
    player->entity_dir = getPlayerDirection();


    // Update Player Position
    player->entity_pos.x += player->entity_dir.x * player->entity_speed.x * delta_time;
    player->entity_pos.y += player->entity_dir.y * player->entity_speed.y * delta_time;


    
    if(IsKeyPressed(KEY_SPACE)) {
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
        }
    } break;
        
    default : fprintf(stderr, "UNREACHABLE: Not a Valid GAMESTATE.\n");
    }
}


void drawMenu(Game *game) {
    Menu *menu = &game->menu;

    if((int)menu->timer % 13 == 0)
        DrawTextureEx(game->menu.stars, (Vector2){100, 10}, 0.0f, 0.08f,  SKYBLUE);
    if((int)menu->timer % 7 == 0)
        DrawTextureEx(game->menu.stars, (Vector2){SCREEN_WIDTH - 200, 10}, 0.0f, 0.08f,  PURPLE);
    if((int)menu->timer % 23 == 0)
        DrawTextureEx(game->menu.stars, (Vector2){200, 100}, 0.0f, 0.08f,  BLUE);
    if((int)menu->timer % 9 == 0)
        DrawTextureEx(game->menu.stars, (Vector2){SCREEN_WIDTH - 300, 450}, 0.0f, 0.1f,  PURPLE);
    if((int)menu->timer % 19 == 0)
        DrawTextureEx(game->menu.stars, (Vector2){190, 400}, 0.0f, 0.1f,  GRAY);
    
    float fontSize = 80.0f;
    float fontSpacing = 4.0f;
    Vector2 textSize = MeasureTextEx(game->font, "SPACE SHOOTER", fontSize, fontSpacing);
    Vector2 textPos = {(float)SCREEN_WIDTH/2 - textSize.x/2, 200};
    DrawTextEx(game->font, "SPACE SHOOTER", textPos, fontSize, fontSpacing, (Color){ 145, 30, 230, 255});

    
    bool mouseOverStart = CheckCollisionPointRec(menu->mousePos, menu->startButton);
    DrawRectangleRec(menu->startButton, mouseOverStart ? GRAY : LIGHTGRAY);

    fontSize = 42.0f;
    fontSpacing = 2.0f;
    textSize = MeasureTextEx(game->font, "START", fontSize, fontSpacing);
    textPos = (Vector2){menu->startButton.x + 100 - textSize.x/2, menu->startButton.y + 5};
    DrawTextEx(game->font, "START", textPos, fontSize, fontSpacing, mouseOverStart ? DARKGRAY :  BLACK);

    
    bool mouseOverExit = CheckCollisionPointRec(menu->mousePos, menu->exitButton);
    DrawRectangleRec(menu->exitButton, mouseOverExit ? GRAY : LIGHTGRAY);
    
    textSize = MeasureTextEx(game->font, "EXIT", fontSize, fontSpacing);
    textPos = (Vector2){menu->exitButton.x + 100 - textSize.x/2, menu->exitButton.y + 5};
    DrawTextEx(game->font, "EXIT", textPos, fontSize, fontSpacing, mouseOverExit ? DARKGRAY :  BLACK);
}

int main() {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
    SetExitKey(KEY_NULL);

    InitAudioDevice();
    
    Game game = {0};
    initializeGame(&game);

    PlayMusicStream(game.menu.bg_song);
    
    while(!WindowShouldClose() && !game.quit) {

        handleGameState(&game);

        if (game.currentState == STATE_GAME) runGamePhysics(&game);
        else if (game.currentState == STATE_MENU) runMenuPhysics(&game);
        
        BeginDrawing();
        {
            ClearBackground(BLACK);

            DrawTextureEx(game.sky_texture, (Vector2){0, 0}, 0, 1.0f * SCREEN_WIDTH / (float)game.sky_texture.height, WHITE);
           
            switch(game.currentState) {
            case STATE_MENU: {
                drawMenu(&game);
            } break;
                
            case STATE_GAME: {
                DrawTextureEx(game.player.entity_texture, game.player.entity_pos, 0.0f, PLAYER_SCALE, WHITE);
                drawBullet(game.player.entity_bullets);
            } break;
                
            default : fprintf(stderr, "UNREACHABLE: Not a Valid GAMESTATE.\n");
            }
        }
        EndDrawing();
    }


    UnloadMusicStream(game.menu.bg_song);
    CloseAudioDevice();
    UnloadFont(game.font);
    UnloadTexture(game.sky_texture);
    UnloadTexture(game.player.entity_texture);
    CloseWindow();
	return 0;
}
