#include "flight.h"

bool eliteCanAttack(const Game *game, int index) {
    const Elite *elite = &game->elites[index];
    // Attack permission depends on whether the pilot can see the emitter.
    // Damage from a pilot bolt is resolved independently in world space.
    if (!elite->unlocked || elite->ship.lives <= 0) return false;
    Vector2 p = GetWorldToScreen2D(elite->ship.position, game->camera);
    if (!onScreen(game->camera, elite->ship.position, 12.0f)) return false;
    bool behindStatus = p.x < 250 && p.y < 204;
    bool behindRadar = game->showRadar && p.x > SCREEN_WIDTH - 225 && p.y < 203;
    return !behindStatus && !behindRadar;
}

static void clearEliteShots(Game *game, int index) {
    for (int i = 0; i < MAX_SHOTS; ++i) {
        if (game->shots[i].owner == index) game->shots[i].life = 0.0f;
    }
}

void damageElite(Game *game, int index, int amount) {
    Elite *elite = &game->elites[index];
    int previousLives = elite->ship.lives;
    elite->ship.lives -= amount;
    elite->ship.hitFlash = 0.11f;
    if (isCarrier(index) && previousLives > elite->maxLives / 2 && elite->ship.lives <= elite->maxLives / 2) {
        elite->ship.lives = elite->maxLives / 2;
        elite->phase = ELITE_IDLE;
        elite->cycle = 2;
        elite->time = 0.45f;
        clearEliteShots(game, index);
        burst(game, elite->ship.position, AMBER, 24);
        return;
    }
    if (elite->ship.lives > 0) return;

    game->payoutPosition = elite->ship.position;
    elite->ship.lives = 0;
    clearEliteShots(game, index);
    burst(game, elite->ship.position, isCarrier(index) ? AMBER : WARDEN_ACCENT, 42);
    PlaySound(game->clearSound);
    if (isCarrier(index)) {
        game->clearBonus = (int)Clamp(3.0f * (120.0f - game->elapsedTime), 0.0f, 250.0f);
        award(game, 500 + game->clearBonus);
        game->missionCleared = true;
        return;
    }

    award(game, 100 + 50 * game->wardensCleared);
    ++game->wardensCleared;
    game->scoutRewards = 0;
    if (game->player.lives < MAX_LIVES) ++game->player.lives;
    game->pulseCharge = fminf(PULSE_RECHARGE, game->pulseCharge + 3.0f);
    if (game->wardensCleared == MAX_ELITES - 1) game->elites[MAX_ELITES - 1].unlocked = true;
}

static int patternCount(const Elite *elite, int index) {
    if (elite->attack == ATTACK_LANES) return 7;
    if (elite->attack == ATTACK_RING) return 18;
    return isCarrier(index) ? (elite->ship.lives <= elite->maxLives / 2 ? 9 : 7) : 5;
}

static float patternAngle(const Elite *elite, int ray, int count) {
    if (elite->attack == ATTACK_RING) {
        const float gap = 0.60f;
        return elite->aim + gap + (2.0f * PI - 2.0f * gap) * (float)ray / (float)(count - 1);
    }
    if (elite->attack == ATTACK_LANES) return elite->aim;
    return elite->aim + ((float)ray - (float)(count - 1) * 0.5f) * 0.15f;
}

static Vector2 muzzleOffset(const Elite *elite, int index, int ray, int count, Vector2 heading) {
    Vector2 offset = Vector2Scale(heading, eliteRadius(index));
    if (elite->attack == ATTACK_LANES) {
        float lane = (float)ray - (float)(count - 1) * 0.5f + (elite->cycle % 2 ? 0.5f : 0.0f);
        offset = Vector2Add(offset, Vector2Scale((Vector2){-heading.y, heading.x}, lane * 56.0f));
    }
    return offset;
}

static void firePattern(Game *game, int index) {
    Elite *elite = &game->elites[index];
    int count = patternCount(elite, index);
    for (int i = 0; i < count; ++i) {
        Vector2 heading = direction(patternAngle(elite, i, count));
        Vector2 muzzle = Vector2Add(elite->ship.position, muzzleOffset(elite, index, i, count, heading));
        addShot(game, index, muzzle, heading, (isCarrier(index) ? 565.0f : 460.0f) * flightRules[game->settings.difficulty].boltSpeed,
                elite->attack == ATTACK_RING ? 6.0f : 5.0f, 2.0f);
    }
    PlaySound(game->clearSound);
}

void updateElite(Game *game, int index) {
    Elite *elite = &game->elites[index];
    if (!elite->unlocked || elite->ship.lives <= 0) return;
    elite->ship.hitFlash = fmaxf(0.0f, elite->ship.hitFlash - STEP);

    if (!eliteCanAttack(game, index)) {
        elite->phase = ELITE_IDLE;
        elite->time = 0.55f;
        return;
    }

    if (elite->phase == ELITE_WARNING && elite->attack == ATTACK_LANCE && elite->time > 0.48f) {
        Vector2 target = Vector2Subtract(game->player.position, elite->ship.position);
        elite->aim = atan2f(target.y, target.x);
    }
    elite->time -= STEP;
    if (elite->phase == ELITE_FIRING) {
        Vector2 endpoint = Vector2Add(elite->ship.position,
                                       Vector2Scale(direction(elite->aim), LANCE_REACH));
        if (distanceToSegment(game->player.position, elite->ship.position, endpoint) <=
            PILOT_RADIUS + LANCE_WIDTH * 0.5f) damagePlayer(game, "Lance strike");
    }
    if (elite->time > 0.0f) return;

    switch (elite->phase) {
    case ELITE_IDLE: {
        static const Attack carrierPatterns[] = { ATTACK_FAN, ATTACK_LANCE, ATTACK_RING, ATTACK_LANES };
        elite->attack = isCarrier(index) ? carrierPatterns[elite->cycle % 4]
                                         : (Attack[]){ ATTACK_FAN, ATTACK_LANCE, ATTACK_LANES }[index];
        Vector2 toPlayer = Vector2Subtract(game->player.position, elite->ship.position);
        elite->aim = atan2f(toPlayer.y, toPlayer.x);
        elite->phase = ELITE_WARNING;
        elite->duration = elite->attack == ATTACK_LANCE ? 0.95f : 0.80f;
        elite->duration *= flightRules[game->settings.difficulty].warningTime;
        elite->time = elite->duration;
        PlaySound(game->warningSound);
        break;
    }
    case ELITE_WARNING:
        if (elite->attack == ATTACK_LANCE) {
            elite->phase = ELITE_FIRING;
            elite->time = 0.30f;
            PlaySound(game->clearSound);
        } else {
            firePattern(game, index);
            elite->phase = ELITE_RECOVERY;
            elite->time = 1.15f;
        }
        break;
    case ELITE_FIRING:
        elite->phase = ELITE_RECOVERY;
        elite->time = 1.35f;
        break;
    case ELITE_RECOVERY:
        ++elite->cycle;
        elite->phase = ELITE_IDLE;
        elite->time = isCarrier(index) && elite->ship.lives <= elite->maxLives / 2 ? 0.22f : 0.55f;
        break;
    }
}

void drawAttack(const Elite *elite, int index) {
    Vector2 center = elite->ship.position;
    if (elite->phase == ELITE_FIRING) {
        Vector2 end = Vector2Add(center, Vector2Scale(direction(elite->aim), LANCE_REACH));
        DrawLineEx(center, end, LANCE_WIDTH + 14.0f, Fade(CORAL, 0.23f));
        DrawLineEx(center, end, LANCE_WIDTH, CORAL);
        DrawLineEx(center, end, 4.0f, (Color){255, 222, 210, 255});
    }
    if (elite->phase != ELITE_WARNING) return;

    float progress = 1.0f - Clamp(elite->time / elite->duration, 0.0f, 1.0f);
    int rayCount = elite->attack == ATTACK_LANCE ? 1 : patternCount(elite, index);
    float length = elite->attack == ATTACK_LANCE ? LANCE_REACH :
                   (elite->attack == ATTACK_RING ? 300.0f : 405.0f);
    for (int i = 0; i < rayCount; ++i) {
        float angle = elite->attack == ATTACK_LANCE ? elite->aim : patternAngle(elite, i, rayCount);
        Vector2 ray = direction(angle);
        Vector2 origin = Vector2Add(center, muzzleOffset(elite, index, i, rayCount, ray));
        for (float offset = 0; offset < length - eliteRadius(index); offset += 27.0f) {
            Vector2 start = Vector2Add(origin, Vector2Scale(ray, offset));
            Vector2 end = Vector2Add(origin, Vector2Scale(ray, fminf(length - eliteRadius(index), offset + 16.0f)));
            DrawLineEx(start, end, 1.5f + progress * 2.0f, Fade(AMBER, 0.38f + progress * 0.48f));
        }
    }
    if (elite->attack == ATTACK_RING) {
        Vector2 gap = Vector2Add(center, Vector2Scale(direction(elite->aim), 127.0f));
        DrawCircleV(gap, 4.0f, CYAN);
    }
    DrawRing(center, eliteRadius(index) + 5.0f, eliteRadius(index) + 8.0f,
             -90.0f, -90.0f + 360.0f * progress, 48, AMBER);
}
