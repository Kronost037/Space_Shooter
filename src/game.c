#include "arena.h"
#include "game.h"
#include "menu.h"
#include "panel.h"

#include <math.h>
#include <string.h>

#define STEP (1.0f / 120.0f)
#define PILOT_RADIUS 17.0f
#define SCOUT_RADIUS 23.0f
#define WARDEN_RADIUS 56.0f
#define CARRIER_RADIUS 73.0f
#define LANCE_WIDTH 17.0f
#define LANCE_REACH 780.0f
#define ENGAGE_RANGE 660.0f
#define CORAL ((Color){255, 86, 111, 255})
#define CYAN ((Color){96, 222, 255, 255})
#define AMBER ((Color){255, 194, 98, 255})
#define WARDEN_ACCENT ((Color){207, 144, 255, 255})

static Vector2 direction(float angle) {
    return (Vector2){ cosf(angle), sinf(angle) };
}

static Vector2 keyboardAxis(int left, int right, int up, int down) {
    return Vector2Normalize((Vector2){
        (float)(IsKeyDown(right) - IsKeyDown(left)),
        (float)(IsKeyDown(down) - IsKeyDown(up))
    });
}

static float distanceToSegment(Vector2 point, Vector2 start, Vector2 end) {
    Vector2 span = Vector2Subtract(end, start);
    float lengthSquared = Vector2LengthSqr(span);
    if (lengthSquared < 0.0001f) return Vector2Distance(point, start);
    float t = Clamp(Vector2DotProduct(Vector2Subtract(point, start), span) / lengthSquared,
                    0.0f, 1.0f);
    return Vector2Distance(point, Vector2Add(start, Vector2Scale(span, t)));
}

static bool onScreen(Camera2D camera, Vector2 point, float margin) {
    Vector2 screen = GetWorldToScreen2D(point, camera);
    return screen.x >= margin && screen.x <= SCREEN_WIDTH - margin &&
           screen.y >= margin && screen.y <= SCREEN_HEIGHT - margin;
}

static bool isCarrier(int index) {
    return index == MAX_ELITES - 1;
}

static float eliteRadius(int index) {
    return isCarrier(index) ? CARRIER_RADIUS : WARDEN_RADIUS;
}

static bool eliteEngaged(const Game *game, int index) {
    const Elite *elite = &game->elites[index];
    if (!elite->unlocked || elite->ship.lives <= 0 ||
        Vector2Distance(game->player.position, elite->ship.position) > ENGAGE_RANGE) return false;

    float radius = eliteRadius(index) + 12.0f;
    Vector2 screen = GetWorldToScreen2D(elite->ship.position, game->camera);
    if (screen.x < radius || screen.x > SCREEN_WIDTH - radius ||
        screen.y < radius || screen.y > SCREEN_HEIGHT - radius) return false;

    // Reserve both HUD corners regardless of the local radar preference.
    bool leftCorner = screen.x < 250.0f + radius && screen.y < 204.0f + radius;
    bool rightCorner = screen.x > SCREEN_WIDTH - 225.0f - radius && screen.y < 203.0f + radius;
    return !leftCorner && !rightCorner;
}

static void burst(Game *game, Vector2 position, Color color, int count) {
    // Cosmetic variation is independent of Raylib's gameplay spawn RNG.
    for (int i = 0; i < MAX_PARTICLES && count > 0; ++i) {
        Particle *particle = &game->particles[i];
        if (particle->life > 0.0f) continue;
        float phase = (float)i * 2.39996f + game->elapsedTime * 3.0f;
        float speed = 45.0f + (float)((i * 37) % 230);
        float life = 0.20f + (float)(i % 7) * 0.045f;
        *particle = (Particle){
            .position = position, .velocity = Vector2Scale(direction(phase), speed),
            .life = life, .totalLife = life, .size = 1.8f + (float)(i % 4), .color = color
        };
        --count;
    }
}

static void award(Game *game, int points) {
    game->score += points;
    game->lastAward = points;
    game->scoreToast = 0.85f;
}

static void damagePlayer(Game *game, const char *cause) {
    if (game->playerHitCooldown > 0.0f || game->player.lives <= 0) return;
    game->damageCause = cause;
    --game->player.lives;
    game->playerHitCooldown = 1.30f;
    burst(game, game->player.position, CORAL, 20);
    PlaySound(game->damageSound);
}

static void destroyScout(Game *game, Ship *scout) {
    game->payoutPosition = scout->position;
    scout->lives = 0;
    ++game->scoutKills;
    if (game->scoutRewards < SCOUT_REWARDS_PER_STAGE) {
        ++game->scoutRewards;
        award(game, 10 + 5 * game->wardensCleared);
    }
    game->pulseCharge = fminf(PULSE_RECHARGE, game->pulseCharge + 0.45f);
    burst(game, scout->position, CORAL, 10);
}

static void clearEliteShots(Game *game, int index) {
    for (int i = 0; i < MAX_SHOTS; ++i) {
        if (game->shots[i].owner == index) game->shots[i].life = 0.0f;
    }
}

static void damageElite(Game *game, int index, int amount) {
    Elite *elite = &game->elites[index];
    int previousLives = elite->ship.lives;
    elite->ship.lives -= amount;
    elite->ship.hitFlash = 0.11f;
    if (isCarrier(index) && previousLives > CARRIER_HP / 2 && elite->ship.lives <= CARRIER_HP / 2) {
        elite->ship.lives = CARRIER_HP / 2;
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

static bool addShot(Game *game, int owner, Vector2 position, Vector2 heading,
                    float speed, float radius, float life) {
    for (int i = 0; i < MAX_SHOTS; ++i) {
        if (game->shots[i].life > 0.0f) continue;
        game->shots[i] = (Shot){
            .position = position, .previous = position, .velocity = Vector2Scale(heading, speed),
            .radius = radius, .life = life, .owner = owner
        };
        return true;
    }
    return false;
}

static int patternCount(const Elite *elite, int index) {
    if (elite->attack == ATTACK_LANES) return 7;
    if (elite->attack == ATTACK_RING) return 18;
    return isCarrier(index) ? (elite->ship.lives <= CARRIER_HP / 2 ? 9 : 7) : 5;
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
        addShot(game, index, muzzle, heading, isCarrier(index) ? 565.0f : 460.0f,
                elite->attack == ATTACK_RING ? 6.0f : 5.0f, 2.0f);
    }
    PlaySound(game->clearSound);
}

static void updateElite(Game *game, int index) {
    Elite *elite = &game->elites[index];
    if (!elite->unlocked || elite->ship.lives <= 0) return;
    elite->ship.hitFlash = fmaxf(0.0f, elite->ship.hitFlash - STEP);

    if (!eliteEngaged(game, index)) {
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
        static const Attack carrierPatterns[] = { ATTACK_FAN, ATTACK_LANCE, ATTACK_RING };
        elite->attack = isCarrier(index) ? carrierPatterns[elite->cycle % 3]
                                         : (Attack[]){ ATTACK_FAN, ATTACK_LANCE, ATTACK_LANES }[index];
        Vector2 toPlayer = Vector2Subtract(game->player.position, elite->ship.position);
        elite->aim = atan2f(toPlayer.y, toPlayer.x);
        elite->phase = ELITE_WARNING;
        elite->duration = elite->attack == ATTACK_LANCE ? 0.95f : 0.80f;
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
        elite->time = isCarrier(index) && elite->ship.lives <= CARRIER_HP / 2 ? 0.22f : 0.55f;
        break;
    }
}

static void activatePulse(Game *game) {
    if (game->pulseCharge < PULSE_RECHARGE) return;
    game->pulseCharge = 0.0f;
    game->pulseAnimation = 0.36f;
    game->pulseOrigin = game->player.position;
    for (int i = 0; i < MAX_SHOTS; ++i) {
        Shot *shot = &game->shots[i];
        if (shot->life <= 0.0f || shot->owner < 0) continue;
        if (Vector2Distance(shot->position, game->pulseOrigin) <= PULSE_RADIUS) shot->life = 0.0f;
    }
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Ship *scout = &game->scouts[i];
        if (scout->lives && Vector2Distance(scout->position, game->pulseOrigin) <= PULSE_RADIUS) {
            destroyScout(game, scout);
        }
    }
    for (int i = 0; i < MAX_ELITES; ++i) {
        if (eliteEngaged(game, i) &&
            Vector2Distance(game->elites[i].ship.position, game->pulseOrigin) <= PULSE_RADIUS) {
            damageElite(game, i, 3);
        }
    }
    burst(game, game->pulseOrigin, CYAN, 25);
    PlaySound(game->pulseSound);
}

static void updateCamera(Game *game, Vector2 move) {
    game->previousCameraTarget = game->camera.target;
    Vector2 target = Vector2Add(game->player.position, Vector2Scale(move, 90.0f));
    Vector2 drift = Vector2Subtract(target, game->camera.target);
    target.x -= Clamp(drift.x, -16, 16);
    target.y -= Clamp(drift.y, -12, 12);
    float halfWidth = SCREEN_WIDTH * 0.5f;
    float halfHeight = SCREEN_HEIGHT * 0.5f;
    target.x = Clamp(target.x, halfWidth - 62.0f, WORLD_WIDTH - halfWidth + 62.0f);
    target.y = Clamp(target.y, halfHeight - 62.0f, WORLD_HEIGHT - halfHeight + 62.0f);
    game->camera.target = Vector2Lerp(game->camera.target, target, 1.0f - expf(-9.0f * STEP));
}

static void spawnScout(Game *game) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (game->scouts[i].lives > 0) continue;
        Vector2 position = {0};
        for (int attempt = 0; attempt < 20; ++attempt) {
            float angle = (float)GetRandomValue(0, 359) * DEG2RAD;
            position = Vector2Add(game->player.position, Vector2Scale(direction(angle), 870.0f));
            if (position.x < 36.0f || position.x > WORLD_WIDTH - 36.0f ||
                position.y < 36.0f || position.y > WORLD_HEIGHT - 36.0f ||
                onScreen(game->camera, position, -36.0f)) continue;
            float speed = (float)GetRandomValue(260, 325);
            game->scouts[i] = (Ship){ .position = position, .previous = position,
                                     .speed = speed, .lives = 1 };
            return;
        }
        return;
    }
}

static void updateScouts(Game *game) {
    game->spawnTimer -= STEP;
    if (game->spawnTimer <= 0.0f) {
        spawnScout(game);
        game->spawnTimer = 2.1f - 0.32f * (float)game->wardensCleared;
    }

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Ship *scout = &game->scouts[i];
        if (!scout->lives) continue;
        Vector2 toward = Vector2Normalize(Vector2Subtract(game->player.position, scout->position));
        Vector2 separation = {0};
        for (int other = 0; other < MAX_ENEMIES; ++other) {
            if (i == other || !game->scouts[other].lives) continue;
            Vector2 away = Vector2Subtract(scout->position, game->scouts[other].position);
            float distance = Vector2Length(away);
            if (distance > 0.01f && distance < 105.0f) {
                separation = Vector2Add(separation, Vector2Scale(away, (105.0f - distance) / (distance * 105.0f)));
            }
        }
        Vector2 desired = Vector2Normalize(Vector2Add(toward, Vector2Scale(separation, 0.95f)));
        scout->heading = Vector2Normalize(Vector2Lerp(scout->heading, desired, 1.0f - expf(-7.0f * STEP)));
        scout->previous = scout->position;
        float speed = scout->speed + 47.0f * (float)game->wardensCleared;
        scout->position = Vector2Add(scout->position, Vector2Scale(scout->heading, speed * STEP));
        if (Vector2Distance(scout->position, game->player.position) <= SCOUT_RADIUS + PILOT_RADIUS) {
            damagePlayer(game, "Scout impact");
        }
    }
}

static bool hitShip(const Shot *shot, const Ship *ship, float radius) {
    Vector2 relativeStart = Vector2Subtract(shot->previous, ship->previous);
    Vector2 relativeEnd = Vector2Subtract(shot->position, ship->position);
    return distanceToSegment((Vector2){0}, relativeStart, relativeEnd) <= shot->radius + radius;
}

static void updateShots(Game *game) {
    for (int i = 0; i < MAX_SHOTS && game->player.lives > 0 && !game->missionCleared; ++i) {
        Shot *shot = &game->shots[i];
        if (shot->life <= 0.0f) continue;
        shot->life -= STEP;
        shot->previous = shot->position;
        shot->position = Vector2Add(shot->position, Vector2Scale(shot->velocity, STEP));
        if (shot->position.x < 0 || shot->position.x > WORLD_WIDTH ||
            shot->position.y < 0 || shot->position.y > WORLD_HEIGHT) {
            shot->life = 0.0f;
            continue;
        }

        if (shot->owner >= 0) {
            if (!onScreen(game->camera, shot->position, -24.0f)) {
                shot->life = 0.0f;
            } else if (hitShip(shot, &game->player, PILOT_RADIUS)) {
                shot->life = 0.0f;
                damagePlayer(game, "Enemy bolt");
            }
            continue;
        }

        for (int scout = 0; scout < MAX_ENEMIES; ++scout) {
            Ship *target = &game->scouts[scout];
            if (!target->lives || !hitShip(shot, target, SCOUT_RADIUS)) continue;
            shot->life = 0.0f;
            destroyScout(game, target);
            break;
        }
        if (shot->life <= 0.0f) continue;

        for (int elite = 0; elite < MAX_ELITES; ++elite) {
            Elite *target = &game->elites[elite];
            if (!eliteEngaged(game, elite) || !hitShip(shot, &target->ship, eliteRadius(elite))) continue;
            shot->life = 0.0f;
            if (isCarrier(elite) && target->phase != ELITE_RECOVERY) {
                burst(game, shot->position, AMBER, 3);
            } else {
                damageElite(game, elite, target->phase == ELITE_RECOVERY ? 2 : 1);
                burst(game, shot->position, WARDEN_ACCENT, 4);
            }
            break;
        }
    }
}

static void step(Game *game, Vector2 move, Vector2 aim, bool pulse) {
    game->elapsedTime += STEP;
    game->scoreToast = fmaxf(0.0f, game->scoreToast - STEP);
    game->pulseAnimation = fmaxf(0.0f, game->pulseAnimation - STEP);
    game->playerHitCooldown = fmaxf(0.0f, game->playerHitCooldown - STEP);
    game->pulseCharge = fminf(PULSE_RECHARGE, game->pulseCharge + STEP);

    Ship *player = &game->player;
    player->previous = player->position;
    Vector2 desired = Vector2Scale(move, player->speed);
    Vector2 oldVelocity = game->velocity;
    float ramp = Vector2LengthSqr(move) > 0 ? 0.08f : 0.06f;
    game->velocity = Vector2MoveTowards(oldVelocity, desired, player->speed * STEP / ramp);
    game->thrust = Vector2Scale(Vector2Subtract(game->velocity, oldVelocity), ramp / (STEP * player->speed));
    player->position = Vector2Add(player->position, Vector2Scale(game->velocity, STEP));
    player->position.x = Clamp(player->position.x, PILOT_RADIUS, WORLD_WIDTH - PILOT_RADIUS);
    player->position.y = Clamp(player->position.y, PILOT_RADIUS, WORLD_HEIGHT - PILOT_RADIUS);
    if (player->position.x <= PILOT_RADIUS || player->position.x >= WORLD_WIDTH - PILOT_RADIUS) game->velocity.x = 0;
    if (player->position.y <= PILOT_RADIUS || player->position.y >= WORLD_HEIGHT - PILOT_RADIUS) game->velocity.y = 0;
    if (Vector2LengthSqr(aim) > 0.0f) game->gunAim = aim;
    Vector2 travel = Vector2Scale(game->velocity, 1.0f / player->speed);
    player->heading = Vector2Normalize((Vector2){0.24f * travel.x, -1.0f});
    updateCamera(game, travel);

    game->fireCooldown -= STEP;
    if (game->fireCooldown <= 0.0f && Vector2LengthSqr(aim) > 0.0f) {
        Vector2 muzzle = Vector2Add(player->position, Vector2Scale(aim, 25.0f));
        if (addShot(game, -1, muzzle, aim, 1100.0f, 4.0f, 0.87f)) {
            game->fireCooldown = 0.16f;
            PlaySound(game->laserSound);
        }
    }

    if (pulse) activatePulse(game);
    if (!game->missionCleared) {
        updateScouts(game);
        for (int i = 0; i < MAX_ELITES && player->lives > 0; ++i) updateElite(game, i);
        if (player->lives > 0) updateShots(game);
    }
}

static void updateEffects(Game *game, float dt) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        Particle *particle = &game->particles[i];
        if (particle->life <= 0.0f) continue;
        particle->life -= dt;
        particle->position = Vector2Add(particle->position, Vector2Scale(particle->velocity, dt));
    }
}

void runGamePhysics(Game *game) {
    float dt = fminf(GetFrameTime(), 0.10f);
    updateEffects(game, dt);
    if (game->player.lives <= 0 || game->missionCleared) {
        game->outcomeTime += dt;
        return;
    }

    Vector2 move = keyboardAxis(KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN);
    Vector2 aim = keyboardAxis(KEY_A, KEY_D, KEY_W, KEY_S);
    if (IsKeyPressed(KEY_SPACE)) game->pulseQueued = true;
    game->accumulator += dt;
    while (game->accumulator >= STEP && game->player.lives > 0 && !game->missionCleared) {
        step(game, move, aim, game->pulseQueued);
        game->pulseQueued = false;
        game->accumulator -= STEP;
    }
}

void resetRun(Game *game) {
    memset(game->scouts, 0, sizeof(game->scouts));
    memset(game->elites, 0, sizeof(game->elites));
    memset(game->shots, 0, sizeof(game->shots));
    memset(game->particles, 0, sizeof(game->particles));
    Vector2 start = { WORLD_WIDTH * 0.50f, WORLD_HEIGHT * 0.50f };
    game->player = (Ship){ .position = start, .previous = start, .heading = {0, -1},
                           .speed = 505.0f, .lives = MAX_LIVES };
    static const Vector2 sites[MAX_ELITES] = {
        {1.5f, 1.5f}, {2.5f, 1.5f}, {2.5f, 2.5f}, {0.5f, 2.5f}
    };
    for (int i = 0; i < MAX_ELITES; ++i) {
        Vector2 position = { sites[i].x * WORLD_SECTOR_WIDTH, sites[i].y * WORLD_SECTOR_HEIGHT };
        game->elites[i] = (Elite){
            .ship = { .position = position, .previous = position,
                      .lives = isCarrier(i) ? CARRIER_HP : WARDEN_HP },
            .unlocked = !isCarrier(i), .time = 0.55f
        };
    }

    game->camera = (Camera2D){ .offset = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
                               .target = start, .zoom = 1.0f };
    game->velocity = game->thrust = (Vector2){0};
    game->gunAim = (Vector2){0,-1};
    game->damageCause = NULL;
    game->previousCameraTarget = start;
    game->accumulator = 0.0f;
    game->outcomeTime = 0.0f;
    game->elapsedTime = 0.0f;
    game->spawnTimer = 0.9f;
    game->fireCooldown = 0.0f;
    game->playerHitCooldown = 0.0f;
    game->pulseCharge = PULSE_RECHARGE;
    game->pulseAnimation = 0.0f;
    game->scoreToast = 0.0f;
    game->score = 0;
    game->lastAward = 0;
    game->scoutKills = 0;
    game->scoutRewards = 0;
    game->clearBonus = 0;
    game->wardensCleared = 0;
    game->scoreSubmitted = false;
    game->missionCleared = false;
    game->pulseQueued = false;
    PauseMusicStream(game->menu->bg_song);
}

static Texture2D loadShip(const char *path) {
    Texture2D texture = LoadTexture(path);
    if (IsTextureValid(texture)) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    }
    return texture;
}

void initializeGame(Game *game) {
    *game = (Game){ .showRadar = true };
    game->sky = loadShip("src/Assets/Siege/deep-field.png");
    game->pilotTexture = loadShip("src/Assets/Siege/pilot.png");
    game->scoutTexture = loadShip("src/Assets/Siege/scout.png");
    game->wardenTexture = loadShip("src/Assets/Siege/warden.png");
    game->carrierTexture = loadShip("src/Assets/Siege/carrier.png");
    game->laserSound = LoadSound("src/Assets/Siege/shot.wav");
    game->warningSound = LoadSound("src/Assets/Siege/warning.wav");
    game->damageSound = LoadSound("src/Assets/Siege/damage.wav");
    game->clearSound = LoadSound("src/Assets/Siege/impact.wav");
    game->pulseSound = LoadSound("src/Assets/Siege/pulse.wav");
    SetSoundVolume(game->laserSound, 0.32f);
    SetSoundVolume(game->warningSound, 0.44f);
    SetSoundVolume(game->damageSound, 0.47f);
    SetSoundVolume(game->clearSound, 0.30f);
    SetSoundVolume(game->pulseSound, 0.40f);

    game->font = LoadFontEx("src/Assets/Siege/Rajdhani-Bold.ttf", 64, NULL, 0);
    SetTextureFilter(game->font.texture, TEXTURE_FILTER_BILINEAR);
    game->menu = malloc(sizeof *game->menu);
    initializeMenu(game->menu);
    leaderboardInit(&game->leaderboard);
    leaderboardLoad(&game->leaderboard);
}

void unloadGameAssets(Game *game) {
    UnloadTexture(game->sky);
    UnloadTexture(game->pilotTexture);
    UnloadTexture(game->scoutTexture);
    UnloadTexture(game->wardenTexture);
    UnloadTexture(game->carrierTexture);
    UnloadSound(game->laserSound);
    UnloadSound(game->warningSound);
    UnloadSound(game->damageSound);
    UnloadSound(game->clearSound);
    UnloadSound(game->pulseSound);
    UnloadFont(game->font);
}

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

static void drawAttack(const Elite *elite, int index) {
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
    drawPanel(game, camera);
}
