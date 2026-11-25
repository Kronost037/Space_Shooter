#include "flight/flight.h"
#include "flight/mission.h"
#include "game.h"
#include "menu.h"

#include <math.h>
#include <string.h>

static Vector2 keyboardAxis(int left, int right, int up, int down) {
    return Vector2Normalize((Vector2){
        (float)(IsKeyDown(right) - IsKeyDown(left)),
        (float)(IsKeyDown(down) - IsKeyDown(up))
    });
}

void burst(Game *game, Vector2 position, Color color, int count) {
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

void award(Game *game, int points) {
    points = (points * flightRules[game->settings.difficulty].scorePercent + 50) / 100;
    game->score += points;
    game->lastAward = points;
    game->scoreToast = 0.85f;
}

void damagePlayer(Game *game, const char *cause) {
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

bool addShot(Game *game, int owner, Vector2 position, Vector2 heading,
                    float speed, float radius, float life) {
    for (int i = 0; i < MAX_SHOTS; ++i) {
        if (game->shots[i].life > 0.0f) continue;
        unsigned targets = 0;
        if (owner < 0) {
            for (int e = 0; e < MAX_ELITES; ++e) {
                if (onScreen(game->camera, game->elites[e].ship.position, -eliteRadius(e) * 1.2f))
                    targets |= 1u << e;
            }
        }
        game->shots[i] = (Shot){
            .visibleTargets = targets,
            .position = position, .previous = position, .velocity = Vector2Scale(heading, speed),
            .radius = radius, .life = life, .owner = owner
        };
        return true;
    }
    return false;
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
        if (game->elites[i].unlocked && game->elites[i].ship.lives > 0 &&
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

static int spawnRange(Game *game, int lo, int hi) {
    uint32_t x = game->spawnSeed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game->spawnSeed = x;
    return lo + (int)(x % (uint32_t)(hi - lo + 1));
}

static void spawnScout(Game *game) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (game->scouts[i].lives > 0) continue;
        Vector2 position = {0};
        for (int attempt = 0; attempt < 20; ++attempt) {
            float angle = (float)spawnRange(game, 0, 359) * DEG2RAD;
            position = Vector2Add(game->player.position, Vector2Scale(direction(angle), 870.0f));
            if (position.x < 36.0f || position.x > WORLD_WIDTH - 36.0f ||
                position.y < 36.0f || position.y > WORLD_HEIGHT - 36.0f ||
                onScreen(game->camera, position, -36.0f)) continue;
            float speed = (float)spawnRange(game, 260, 325);
            game->scouts[i] = (Ship){ .position = position, .previous = position,
                                     .speed = speed, .lives = 1 };
            return;
        }
        return;
    }
}

static void updateScouts(Game *game) {
    const FlightRules *rules = &flightRules[game->settings.difficulty];
    Elite *carrier = &game->elites[MAX_ELITES - 1];
    bool duel = carrier->unlocked && carrier->ship.lives > 0;
    int escorts = 0;
    for (int i = 0; duel && i < MAX_ENEMIES; ++i) {
        Ship *scout = &game->scouts[i];
        if (!scout->lives) continue;
        if (escorts++ == 0) continue;
        scout->previous = scout->position;
        Vector2 away = Vector2Normalize(Vector2Subtract(scout->position, game->player.position));
        if (Vector2LengthSqr(away) < 0.01f) away = (Vector2){1,0};
        scout->heading = away;
        scout->position = Vector2Add(scout->position, Vector2Scale(away, 650 * STEP));
        if (!onScreen(game->camera,scout->position,-40)) scout->lives = 0;
    }
    game->spawnTimer -= STEP;
    if (game->spawnTimer <= 0.0f) {
        if (!duel || escorts == 0) spawnScout(game);
        game->spawnTimer = duel ? 12.0f : (2.1f - 0.32f * game->wardensCleared) * rules->spawnInterval;
    }

    int activeEscorts = 0;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Ship *scout = &game->scouts[i];
        if (!scout->lives) continue;
        if (duel && activeEscorts++ > 0) continue;
        Vector2 pursuit = game->mission.stage == MISSION_ESCORT && i % 3 == 0 ?
                          game->mission.transport : game->player.position;
        Vector2 toward = Vector2Normalize(Vector2Subtract(pursuit, scout->position));
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
        float speed = (scout->speed + 47.0f * game->wardensCleared) * rules->scoutSpeed * (duel ? 0.38f : 1.0f);
        scout->position = Vector2Add(scout->position, Vector2Scale(scout->heading, speed * STEP));
        if (Vector2Distance(scout->position, game->player.position) <= SCOUT_RADIUS + PILOT_RADIUS) {
            damagePlayer(game, "Scout impact");
        }
    }
}

// Return the first contact along this tick's relative motion.
static float contactTime(const Shot *shot, const Ship *ship, float radius) {
    Vector2 start = Vector2Subtract(shot->previous, ship->previous);
    Vector2 end = Vector2Subtract(shot->position, ship->position);
    Vector2 travel = Vector2Subtract(end,start);
    float combined = shot->radius + radius;
    float c = Vector2LengthSqr(start) - combined * combined;
    if (c <= 0) return 0;
    float a = Vector2LengthSqr(travel), b = Vector2DotProduct(start,travel);
    float discriminant = b*b - a*c;
    if (a == 0 || b >= 0 || discriminant < 0) return INFINITY;
    float t = c / (-b + sqrtf(discriminant));
    return t <= 1 ? t : INFINITY;
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
            } else if (isfinite(contactTime(shot, &game->player, PILOT_RADIUS))) {
                shot->life = 0.0f;
                damagePlayer(game, "Enemy bolt");
            }
            continue;
        }

        float first = INFINITY;
        int scoutHit = -1, eliteHit = -1;
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (!game->scouts[i].lives) continue;
            float t = contactTime(shot,&game->scouts[i],SCOUT_RADIUS);
            if (t < first) { first = t; scoutHit = i; }
        }
        for (int i = 0; i < MAX_ELITES; ++i) {
            Elite *target = &game->elites[i];
            if (!target->unlocked || target->ship.lives <= 0) continue;
            float t = contactTime(shot,&target->ship,eliteRadius(i));
            if (t >= first) continue;
            if (!(shot->visibleTargets & (1u << i)) &&
                !onScreen(game->camera,target->ship.position,-eliteRadius(i)*1.2f)) continue;
            first = t;
            eliteHit = i;
        }
        if (eliteHit >= 0) {
            damageElite(game,eliteHit,game->elites[eliteHit].phase == ELITE_RECOVERY ? 2 : 1);
            burst(game,Vector2Lerp(shot->previous,shot->position,first),CYAN,4);
        } else if (scoutHit >= 0) destroyScout(game,&game->scouts[scoutHit]);
        if (isfinite(first)) shot->life = 0;
    }
}

static void turnPilot(Game *game, Vector2 move) {
    game->previousHullAngle = game->hullAngle;
    if (Vector2LengthSqr(move) > 0.0f) {
        float target = atan2f(move.y, move.x);
        float error = remainderf(target - game->hullAngle, 2 * PI);
        // A damped spring leads into the turn and settles with a slight overshoot.
        game->turnSpeed += (error * 125.0f - game->turnSpeed * 17.0f) * STEP;
    } else {
        game->turnSpeed *= expf(-19.0f * STEP);
    }
    game->hullAngle += game->turnSpeed * STEP;
}

static void step(Game *game, Vector2 move, Vector2 aim, bool pulse) {
    game->dispatchTime = fmaxf(0.0f, game->dispatchTime - STEP);
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
    turnPilot(game, move);
    updateCamera(game, travel);

    game->fireCooldown -= STEP;
    if (game->fireCooldown <= 0.0f && Vector2LengthSqr(aim) > 0.0f) {
        Vector2 muzzle = Vector2Add(player->position, Vector2Scale(aim, 25.0f));
        if (addShot(game, -1, muzzle, aim, 1100.0f, 4.0f, 1.75f)) {
            game->fireCooldown = 0.16f;
            PlaySound(game->laserSound);
        }
    }

    if (pulse) activatePulse(game);
    if (!game->missionCleared) {
        updateScouts(game);
        for (int i = 0; i < MAX_ELITES && player->lives > 0; ++i) updateElite(game, i);
        if (player->lives > 0) updateShots(game);
        if (player->lives > 0 && !game->missionCleared) advanceMission(game, STEP);
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

void advanceFlight(Game *game, double frameTime, Vector2 move, Vector2 aim, bool pulse) {
    float effectTime = (float)fmin(frameTime, 0.10);
    updateEffects(game, effectTime);
    if (game->player.lives <= 0 || game->missionCleared || game->timedOut) {
        game->outcomeTime += effectTime;
        return;
    }

    double limit = game->settings.timerMinutes * 60.0;
    double activeTime = game->settings.timerEnabled ? fmin(frameTime, limit - game->elapsedTime) : frameTime;
    game->elapsedTime += activeTime;
    // Catch-up work is bounded, but a slow frame still counts against the flight clock.
    game->accumulator += (float)fmin(activeTime, 0.10);
    game->pulseQueued |= pulse;
    while (game->accumulator >= STEP && game->player.lives > 0 && !game->missionCleared) {
        ++game->flightTick;
        step(game, move, aim, game->pulseQueued);
        game->pulseQueued = false;
        game->accumulator -= STEP;
    }
    game->timedOut = game->settings.timerEnabled && game->elapsedTime >= limit &&
                     game->player.lives > 0 && !game->missionCleared;
}

void runGamePhysics(Game *game) {
    Vector2 move = keyboardAxis(KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN);
    Vector2 aim = keyboardAxis(KEY_A, KEY_D, KEY_W, KEY_S);
    advanceFlight(game, GetFrameTime(), move, aim, IsKeyPressed(KEY_SPACE));
}

void resetRun(Game *game) {
    resetRunSeeded(game, (uint32_t)GetRandomValue(1, 2147483646));
}

void resetRunSeeded(Game *game, uint32_t seed) {
    game->spawnSeed = seed ? seed : 1u;
    memset(game->scouts, 0, sizeof(game->scouts));
    memset(game->elites, 0, sizeof(game->elites));
    memset(game->shots, 0, sizeof(game->shots));
    memset(game->particles, 0, sizeof(game->particles));
    Vector2 start = { WORLD_WIDTH * 0.50f, WORLD_HEIGHT * 0.50f };
    startMission(game, start);
    game->player = (Ship){ .position = start, .previous = start, .heading = {0, -1},
                           .speed = 505.0f, .lives = MAX_LIVES };
    static const Vector2 sites[MAX_ELITES] = {
        {1.5f, 1.5f}, {2.5f, 1.5f}, {2.5f, 2.5f}, {0.5f, 2.5f}
    };
    for (int i = 0; i < MAX_ELITES; ++i) {
        Vector2 position = { sites[i].x * WORLD_SECTOR_WIDTH, sites[i].y * WORLD_SECTOR_HEIGHT };
        const FlightRules *rules = &flightRules[game->settings.difficulty];
        int hull = isCarrier(i) ? rules->carrierHull : rules->wardenHull;
        game->elites[i] = (Elite){
            .maxLives = hull,
            .patrolCenter = position,
            .patrolPhase = (float)((seed >> (i * 8)) & 255u) * (2 * PI / 256),
            .ship = { .position = position, .previous = position,
                      .lives = hull },
            .time = 0.55f
        };
    }

    game->camera = (Camera2D){ .offset = { SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f },
                               .target = start, .zoom = 1.0f };
    game->velocity = game->thrust = (Vector2){0};
    game->gunAim = (Vector2){0,-1};
    game->hullAngle = game->previousHullAngle = -PI * 0.5f;
    game->turnSpeed = 0;
    game->scene = SCENE_NONE;
    game->sceneTime = 0;
    game->damageCause = NULL;
    game->previousCameraTarget = start;
    game->accumulator = 0.0f;
    game->outcomeTime = 0.0f;
    game->elapsedTime = 0.0f;
    game->flightTick = 0;
    game->dispatchTime = 0;
    game->dispatch = NULL;
    game->timedOut = false;
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
    *game = (Game){ .showRadar = true, .settings = DEFAULT_SETTINGS };
    game->transportTexture = loadShip("src/Assets/Siege/mercy-six.png");
    game->gateTexture = loadShip("src/Assets/Siege/jump-gate.png");
    game->hatch = loadShip("src/Assets/Siege/hatch.png");
    game->crewPortraits = loadShip("src/Assets/Siege/crew.png");
    game->drydock = loadShip("src/Assets/Siege/drydock.png");
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

    game->bodyFont = LoadFontEx("src/Assets/Siege/Barlow-Medium.ttf", 72, NULL, 0);
    GenTextureMipmaps(&game->bodyFont.texture);
    SetTextureFilter(game->bodyFont.texture, TEXTURE_FILTER_TRILINEAR);
    game->menu = malloc(sizeof *game->menu);
    initializeMenu(game->menu);
    leaderboardInit(&game->leaderboard);
    leaderboardLoad(&game->leaderboard);
}

void shutdownGame(Game *game) {
    UnloadTexture(game->transportTexture);
    UnloadTexture(game->gateTexture);
    UnloadTexture(game->sky);
    UnloadTexture(game->drydock);
    UnloadTexture(game->crewPortraits);
    UnloadTexture(game->hatch);
    UnloadTexture(game->pilotTexture);
    UnloadTexture(game->scoutTexture);
    UnloadTexture(game->wardenTexture);
    UnloadTexture(game->carrierTexture);
    UnloadSound(game->laserSound);
    UnloadSound(game->warningSound);
    UnloadSound(game->damageSound);
    UnloadSound(game->clearSound);
    UnloadSound(game->pulseSound);
    UnloadFont(game->bodyFont);
    UnloadMusicStream(game->menu->bg_song);
    UnloadSound(game->menu->hover_sfx);
    UnloadSound(game->menu->click_sfx);
    free(game->menu);
    leaderboardShutdown(&game->leaderboard);
}
