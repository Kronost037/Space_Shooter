#include "flight.h"
#include "../arena.h"

static void drawShip(Texture2D texture, Vector2 position, Vector2 heading, float size,
                     Color tint, Color fallback) {
    float angle = atan2f(heading.y, heading.x) * RAD2DEG + 90.0f;
    if (!IsTextureValid(texture)) {
        DrawCircleV(position, size * 0.31f, fallback);
        DrawLineEx(position, Vector2Add(position, Vector2Scale(heading, size * 0.50f)), 4.0f, tint);
        return;
    }
    float width = (float)texture.width;
    float height = (float)texture.height;
    float scale = size / fmaxf(width, height);
    Vector2 origin = { width * scale * 0.5f, height * scale * 0.5f };
    DrawTexturePro(texture, (Rectangle){0, 0, width, height},
                   (Rectangle){position.x, position.y, width * scale, height * scale}, origin, angle, tint);
}

void drawGame(Game *game) {
    float blend = Clamp(game->accumulator / STEP, 0.0f, 1.0f);
    Camera2D camera = game->camera;
    camera.target = Vector2Lerp(game->previousCameraTarget, camera.target, blend);
    drawArenaBackdrop(camera, game->sky);

    BeginMode2D(camera);
    drawSectorLandmarks(game, camera);

    for (int i = 0; i < MAX_ELITES; ++i) {
        const Elite *elite = &game->elites[i];
        if (!elite->unlocked || !elite->ship.lives) continue;
        drawAttack(elite, i);
        Color tint = elite->ship.hitFlash > 0.0f ? Fade(AMBER, 0.95f) : WHITE;
        Color accent = isCarrier(i) ? AMBER : WARDEN_ACCENT;
        Texture2D texture = isCarrier(i) ? game->carrierTexture : game->wardenTexture;
        if (elite->phase == ELITE_RECOVERY) {
            DrawCircleGradient(elite->ship.position,
                               eliteRadius(i) + 35.0f, Fade(CYAN, 0.36f), Fade(CYAN, 0.0f));
        }
        drawShip(texture, elite->ship.position, (Vector2){0, -1}, eliteRadius(i) * 2.4f, tint, accent);
        if (isCarrier(i) && elite->phase != ELITE_RECOVERY) {
            float radius = eliteRadius(i);
            DrawRing(elite->ship.position, radius + 9.0f, radius + 11.0f,
                     30.0f, 150.0f, 24, Fade(AMBER, 0.42f));
            DrawRing(elite->ship.position, radius + 9.0f, radius + 11.0f,
                     210.0f, 330.0f, 24, Fade(AMBER, 0.42f));
        }
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        const Ship *scout = &game->scouts[i];
        if (!scout->lives) continue;
        drawShip(game->scoutTexture, Vector2Lerp(scout->previous, scout->position, blend),
                 scout->heading, 64.0f, WHITE, CORAL);
    }

    for (int i = 0; i < MAX_SHOTS; ++i) {
        const Shot *shot = &game->shots[i];
        if (shot->life <= 0.0f) continue;
        Color color = shot->owner < 0 ? CYAN : CORAL;
        Vector2 p = Vector2Lerp(shot->previous, shot->position, blend);
        Vector2 tail = Vector2Subtract(p, Vector2Scale(Vector2Normalize(shot->velocity), 21.0f));
        DrawLineEx(tail, p, shot->radius * 2.0f + 4.0f, Fade(color, 0.23f));
        DrawLineEx(tail, p, shot->radius, color);
        DrawCircleV(p, shot->radius, color);
        DrawCircleV(p, shot->radius * 0.43f, RAYWHITE);
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        const Particle *particle = &game->particles[i];
        if (particle->life > 0.0f) {
            DrawCircleV(particle->position, particle->size,
                        Fade(particle->color, particle->life / particle->totalLife));
        }
    }
    if (game->pulseAnimation > 0.0f) {
        float progress = 1.0f - game->pulseAnimation / 0.36f;
        float radius = 28.0f + (PULSE_RADIUS - 28.0f) * (1.0f - powf(1.0f - progress, 3.0f));
        DrawRing(game->pulseOrigin, radius - 4.0f, radius, 0, 360, 90, Fade(CYAN, 1.0f - progress));
    }

    Vector2 pilot = Vector2Lerp(game->player.previous, game->player.position, blend);
    if (game->player.lives > 0) {
        Vector2 exhaust = Vector2Subtract(pilot, Vector2Scale(game->thrust, 27.0f));
        DrawCircleGradient(exhaust, 19.0f, Fade(CYAN, 0.55f * Clamp(Vector2Length(game->thrust),0,1)), Fade(CYAN, 0.0f));
        drawShip(game->pilotTexture, pilot, game->player.heading, 66.0f, WHITE, CYAN);
        Vector2 muzzle = Vector2Add(pilot, Vector2Scale(game->gunAim, 25));
        DrawLineEx(pilot, muzzle, 5, (Color){67,101,124,255});
        DrawCircleV(muzzle, 2.8f, CYAN);
        if (game->playerHitCooldown > 0.0f) {
            DrawRing(pilot, 32.0f, 34.5f, 0, 360, 32, Fade(CYAN, game->playerHitCooldown / 1.30f));
        }
    }
    EndMode2D();

}
