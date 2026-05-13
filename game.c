#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define UFO_SCALE 0.03f
#define MAX_BULLETS 10

typedef struct S_ufo {
    Vector2 ufo_pos;
    Vector2 ufo_speed;
    Vector2 ufo_dir;
    Texture2D ufo_texture;
} Ufo;

typedef struct S_projectile {
    Vector2 projectile_pos_start;
    Vector2 projectile_speed;
} Projectile;

void initializePlayer(Ufo *player) {
    player->ufo_pos = (Vector2){0, 0};
    player->ufo_speed = (Vector2){400, 400};
    player->ufo_dir = (Vector2){0, 0};
    player->ufo_texture = LoadTexture("ufo.png");
}

void makeBullet(Ufo *player, Projectile *bullets) {
    Projectile bullet;
    bullet.projectile_pos_start.x = player->ufo_pos.x + player->ufo_texture.width /2.0 * UFO_SCALE;
    bullet.projectile_pos_start.y = player->ufo_pos.y;
    
    bullet.projectile_speed = (Vector2){0, 700};

    for(int i = 0; i < MAX_BULLETS; i++) {
        if(bullets[i].projectile_pos_start.y == 0) {
            bullets[i] = bullet;
            break;
        }
    }
}

void fireBullet(Projectile *bullets, int bullets_count) {
    for(int i = 0; i < MAX_BULLETS; i++) {
        if(bullets[i].projectile_pos_start.y > 0) {
            Vector2 start = bullets[i].projectile_pos_start;
            Vector2 end = bullets[i].projectile_pos_start;
            end.y -= 10;
            DrawLineV(start, end, RED);
        }
    }
}

int main() {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");

    Ufo player = {0};
    initializePlayer(&player);

    Projectile bullets[MAX_BULLETS] = {0};
    int bullets_count = 0;
    
    while(!WindowShouldClose()) {
        float delta_time = GetFrameTime();

        // Movements
        player.ufo_dir = (Vector2){0, 0};
        
        if(IsKeyDown(KEY_D)) {
            player.ufo_dir.x += 1;    
        }
        if(IsKeyDown(KEY_A)) {
            player.ufo_dir.x -= 1;
        }
        if(IsKeyDown(KEY_S)) {
            player.ufo_dir.y += 1;
        }
        if(IsKeyDown(KEY_W)) {
            player.ufo_dir.y -= 1;
        }

        // Update Player Position
        player.ufo_dir = Vector2Normalize(player.ufo_dir);
        player. ufo_pos.x += player.ufo_dir.x * player.ufo_speed.x * delta_time;
        player.ufo_pos.y += player.ufo_dir.y * player.ufo_speed.y * delta_time;

        // Bullets
        if(IsKeyPressed(KEY_SPACE) && bullets_count <= MAX_BULLETS) {
            makeBullet(&player, bullets);
            bullets_count++;
        }

        // Update Bullets Position
        for(int i = 0; i < MAX_BULLETS; i++) {
            bullets[i].projectile_pos_start.y -= bullets[i].projectile_speed.y * delta_time;
        }
        
        // Collision
        float ufo_scaledWidth = player.ufo_texture.width * UFO_SCALE;
        float ufo_scaledHeight = player.ufo_texture.height * UFO_SCALE;
        
        if (player.ufo_pos.x < 0)
            player.ufo_pos.x = 0;
        if (player.ufo_pos.x > SCREEN_WIDTH - ufo_scaledWidth)
            player.ufo_pos.x = SCREEN_WIDTH - ufo_scaledWidth;
        if (player.ufo_pos.y < 0)
            player.ufo_pos.y = 0;
        if (player.ufo_pos.y > SCREEN_HEIGHT - ufo_scaledHeight)
            player.ufo_pos.y = SCREEN_HEIGHT - ufo_scaledHeight;

        for(int i = 0; i < MAX_BULLETS; i++) {
            if(bullets[i].projectile_pos_start.y < 0) {
                bullets[i].projectile_pos_start.y = 0;
                bullets_count--;
            } 
        }
        
        // Display
        BeginDrawing();
        {
            ClearBackground(BLACK);
            DrawTextureEx(player.ufo_texture, player.ufo_pos, 0.0f, UFO_SCALE, WHITE);
            fireBullet(bullets, bullets_count);
        }
        EndDrawing();
    }
    
    UnloadTexture(player.ufo_texture);
    CloseWindow();
	return 0;
}
