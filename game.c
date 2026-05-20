#include <math.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>

#define SCREEN_WIDTH 1500
#define SCREEN_HEIGHT 900
#define PLAYER_SCALE 0.03f
#define MAX_BULLETS 10

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

typedef struct S_game {
    Texture2D sky_texture;
    Entity player;
} Game;

void initializeGame(Game *game) {
    game->player = (Entity){0};
    initializePlayer(&game->player);
    game->sky_texture = LoadTexture("Assets/sky.png");
}

void makeBullet(Entity *player, Projectile *bullets) {
    Projectile bullet = {0};
    
    float halfPlayerWidth = (player->entity_texture.width * PLAYER_SCALE) / 2.0f;
    float halfPlayerHeight = (player->entity_texture.height * PLAYER_SCALE) / 2.0f;
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

void fireBullet(Projectile *bullets) {
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
    
    float entity_scaledWidth = player->entity_texture.width * PLAYER_SCALE;
    float entity_scaledHeight = player->entity_texture.height * PLAYER_SCALE;
    
    if (player->entity_pos.x < 0)
        player->entity_pos.x = 0;
    if (player->entity_pos.x > SCREEN_WIDTH - entity_scaledWidth)
        player->entity_pos.x = SCREEN_WIDTH - entity_scaledWidth;
    if (player->entity_pos.y < 0)
        player->entity_pos.y = 0;
    if (player->entity_pos.y > SCREEN_HEIGHT - entity_scaledHeight)
        player->entity_pos.y = SCREEN_HEIGHT - entity_scaledHeight;
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (player->entity_bullets[i].projectile_pos_start.y < 0 ||
            player->entity_bullets[i].projectile_pos_start.y > SCREEN_HEIGHT ||
            player->entity_bullets[i].projectile_pos_start.x < 0 ||
            player->entity_bullets[i].projectile_pos_start.x > SCREEN_WIDTH) {
            
            player->entity_bullets[i].active = false;
        }
    }
}

void runPhysics(Game *game) {
    float delta_time = GetFrameTime();

    Entity *player = &game->player;
    
    player->entity_dir = getPlayerDirection();
    
    player->entity_pos.x += player->entity_dir.x * player->entity_speed.x * delta_time;
    player->entity_pos.y += player->entity_dir.y * player->entity_speed.y * delta_time;

    if(IsKeyPressed(KEY_SPACE)) {
        makeBullet(player, player->entity_bullets);
    }
    
    for(int i = 0; i < MAX_BULLETS; i++) {
        Projectile *bullet = &player->entity_bullets[i];
        if(bullet->active) {
            bullet->projectile_pos_start.x += bullet->projectile_dir.x * bullet->projectile_speed.x * delta_time;
            bullet->projectile_pos_start.y += bullet->projectile_dir.y * bullet->projectile_speed.y * delta_time;
        }
    }

    handleCollision(game);
}

int main() {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");

    Game game = {0};
    initializeGame(&game);

    while(!WindowShouldClose()) {
        
        runPhysics(&game);

        BeginDrawing();
        {
            ClearBackground(BLACK);
            DrawTextureEx(game.sky_texture, (Vector2){0, 0}, 0, 1.0f * SCREEN_WIDTH / game.sky_texture.height, WHITE);
            DrawTextureEx(game.player.entity_texture, game.player.entity_pos, 0.0f, PLAYER_SCALE, WHITE);
            fireBullet(game.player.entity_bullets);
        }
        EndDrawing();
    }

    UnloadTexture(game.player.entity_texture);
    CloseWindow();
	return 0;
}
