// Exercise the production fixed-step loop and the native window renderer under Xvfb.
#include "../src/flight/flight.h"
#include "../src/ui/display.h"
#include "../src/menu.h"
#include "../src/flight/mission.h"
#include "../src/settings.h"
#include <string.h>
#include <assert.h>
#include <rlgl.h>

static void acquireFixes(Game *game) {
    assert(game->mission.stage == MISSION_SURVEY && !game->elites[0].unlocked);
    for (int i = 0; i < 2; ++i) {
        game->player.previous = game->player.position = game->mission.waypoint;
        advanceFlight(game, STEP, (Vector2){0}, (Vector2){0}, false);
    }
    assert(game->mission.stage == MISSION_RELAYS && game->elites[0].unlocked);
    game->camera.target = game->previousCameraTarget = game->player.position;
}

static void escort(Game *game) {
    assert(game->mission.stage == MISSION_ESCORT && !game->elites[3].unlocked);
    game->scouts[0] = (Ship){.lives = 1, .position = game->mission.transport};
    Vector2 origin = game->mission.transport;
    game->player.position = origin;
    advanceMission(game, STEP);
    assert(game->mission.contested && Vector2Equals(origin, game->mission.transport));
    memset(game->scouts, 0, sizeof game->scouts);
    game->player.position.x += 350;
    advanceMission(game, STEP);
    assert(Vector2Equals(origin, game->mission.transport));
    for (int i = 0; i < 800 && game->mission.stage == MISSION_ESCORT; ++i) {
        game->player.position = game->mission.transport;
        advanceMission(game, STEP);
    }
    assert(game->mission.stage == MISSION_CARRIER && game->elites[3].unlocked);
}

static void placeElite(Game *game, int index, Vector2 at) {
    Elite *elite = &game->elites[index];
    elite->ship.previous = elite->ship.position = at;
}

static void contact(Game *game, int owner, Vector2 at) {
    assert(addShot(game, owner, at, (Vector2){1,0}, 1, 4, 1));
    advanceFlight(game, STEP, (Vector2){0}, (Vector2){0}, false);
}

static void testFlight(Game *game) {
    for (int d = EASY; d <= INSANE; ++d) {
        game->settings.difficulty = (Difficulty)d;
        resetRun(game);
        acquireFixes(game);
        int surveyScore = game->score;
        Elite *warden = &game->elites[0];
        Vector2 edge = GetScreenToWorld2D((Vector2){10,16}, game->camera);
        placeElite(game, 0, edge);
        assert(!eliteCanAttack(game, 0));
        contact(game, -1, edge);
        assert(warden->ship.lives == flightRules[d].wardenHull - 1);
        Vector2 far = GetScreenToWorld2D((Vector2){1450,520}, game->camera);
        placeElite(game, 0, far);
        assert(eliteCanAttack(game, 0));
        contact(game, -1, far);
        assert(warden->ship.lives == warden->maxLives - 2);
        Elite *carrier = &game->elites[MAX_ELITES - 1];
        placeElite(game, MAX_ELITES - 1, Vector2Add(game->player.position, (Vector2){270,0}));
        contact(game, -1, carrier->ship.position);
        assert(carrier->ship.lives == carrier->maxLives);
        memset(game->shots, 0, sizeof game->shots);
        carrier->unlocked = true;
        carrier->phase = ELITE_IDLE;
        contact(game, -1, carrier->ship.position);
        carrier->phase = ELITE_RECOVERY;
        contact(game, -1, carrier->ship.position);
        assert(carrier->ship.lives == carrier->maxLives - 3);
        for (int i = 0; i < MAX_ENEMIES; ++i)
            game->scouts[i] = (Ship){.position = far, .previous = far, .speed = 300, .lives = 1};
        game->spawnTimer = 0;
        advanceFlight(game, STEP, (Vector2){0}, (Vector2){0}, false);
        int escorts = 0;
        for (int i = 0; i < MAX_ENEMIES; ++i) escorts += game->scouts[i].lives;
        assert(escorts == 1 && game->scoutKills == 0 && game->score == surveyScore);

        game->settings.timerEnabled = true;
        game->settings.timerMinutes = 1;
        game->elapsedTime = 59.25;
        advanceFlight(game, 2.5, (Vector2){0}, (Vector2){0}, false);
        assert(game->timedOut && game->elapsedTime == 60 && !game->missionCleared);
        game->settings.timerEnabled = false;
        resetRun(game);
        assert(!game->timedOut && game->elapsedTime == 0 && game->score == 0);
        acquireFixes(game);
        for (int i = 0; i < 3; ++i) damageElite(game, i, 200);
        assert(game->wardensCleared == 3);
        escort(game);
        damageElite(game, 3, 200);
        assert(game->elites[3].ship.lives == game->elites[3].maxLives / 2);
        damageElite(game, 3, 200);
        assert(game->missionCleared);
    }
    puts("PASS: four difficulties, visible-edge hits, HUD and lock gates, armor, escort cap, timer, reset and clear");
}

static void testTimingAndMovement(Game *game) {
    static const int rates[] = {30,60,120,240};
    Vector2 reference = {0};
    game->settings = DEFAULT_SETTINGS;
    for (unsigned r = 0; r < sizeof rates / sizeof *rates; ++r) {
        resetRun(game);
        for (int frame = 0; frame < rates[r]; ++frame)
            advanceFlight(game, 1.0 / rates[r], (Vector2){1,0}, (Vector2){0,-1}, false);
        assert(fabs(game->elapsedTime - 1) < 0.00001);
        if (r == 0) reference = game->player.position;
        assert(Vector2Distance(reference, game->player.position) < 5);
        assert(game->velocity.x > 500 && game->velocity.y == 0);
        for (int frame = 0; frame < rates[r]; ++frame)
            advanceFlight(game, 1.0 / rates[r], (Vector2){0}, (Vector2){0}, false);
        assert(Vector2Length(game->velocity) < 0.01f);
    }
    resetRun(game);
    game->player.position = (Vector2){WORLD_WIDTH - PILOT_RADIUS, PILOT_RADIUS};
    advanceFlight(game, 1, Vector2Normalize((Vector2){1,-1}), (Vector2){0}, false);
    assert(game->player.position.x == WORLD_WIDTH - PILOT_RADIUS && game->player.position.y == PILOT_RADIUS);
    assert(Vector2Length(game->velocity) == 0);
    puts("PASS: frame cadence 30/60/120/240, movement, braking, corner limits, bounded catch-up");
}

static void testProjectilePool(Game *game) {
    resetRun(game);
    for (int i = 0; i < MAX_SHOTS; ++i)
        assert(addShot(game, 0, Vector2Add(game->player.position,(Vector2){125,0}), (Vector2){0}, 0, 5, 1));
    assert(!addShot(game, -1, game->player.position, (Vector2){1,0}, 1, 5, 1));
    advanceFlight(game, STEP, (Vector2){0}, (Vector2){0}, true);
    for (int i = 0; i < MAX_SHOTS; ++i) assert(game->shots[i].life <= 0);
    assert(game->pulseCharge == 0);
    advanceFlight(game, STEP, (Vector2){0}, (Vector2){0}, true);
    assert(game->pulseCharge > 0);
    assert(addShot(game, -1, game->player.position, (Vector2){1,0}, 1, 5, 1));

    resetRun(game);
    acquireFixes(game);
    const Vector2 hidden = Vector2Add(game->camera.target, (Vector2){1000,0});
    placeElite(game, 0, hidden);
    contact(game, -1, hidden);
    assert(game->elites[0].ship.lives == game->elites[0].maxLives);
    memset(game->shots, 0, sizeof game->shots);
    Vector2 visible = Vector2Add(game->camera.target, (Vector2){740,0});
    placeElite(game, 0, visible);
    assert(addShot(game, -1, Vector2Subtract(visible,(Vector2){90,0}), (Vector2){1,0}, 20000, 4, 1));
    game->camera.target.x -= 220;
    advanceFlight(game, STEP, (Vector2){0}, (Vector2){0}, false);
    assert(game->elites[0].ship.lives == game->elites[0].maxLives - 1);
    puts("PASS: saturated pool, pulse discharge, cooldown, blind fire and swept hit after camera movement");
}

static void testSeededWorlds(Game *game) {
    // Copies borrow loaded assets. Each simulation owns its entity arrays and spawn generator.
    Game other = *game;
    game->settings = other.settings = DEFAULT_SETTINGS;
    resetRunSeeded(game, 12345);
    resetRunSeeded(&other, 12345);
    for (int i = 0; i < 600; ++i) {
        advanceFlight(game, STEP, (Vector2){0,1}, (Vector2){1,0}, false);
        advanceFlight(&other, STEP, (Vector2){0,1}, (Vector2){1,0}, false);
    }
    assert(game->spawnSeed == other.spawnSeed && game->spawnSeed != 12345);
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        assert(game->scouts[i].lives == other.scouts[i].lives);
        assert(Vector2Equals(game->scouts[i].position, other.scouts[i].position));
    }
    puts("PASS: interleaved same-seed worlds reproduce patrol placements");
}

static void testPatterns(Game *game) {
    for (int d = EASY; d <= INSANE; ++d) {
        game->settings.difficulty = (Difficulty)d;
        resetRun(game);
        Elite *carrier = &game->elites[3];
        carrier->unlocked = true;
        placeElite(game, 3, Vector2Add(game->player.position,(Vector2){300,0}));
        for (int phase = 0; phase < 2; ++phase) {
            unsigned attacks = 0;
            int emittedRings = 0;
            for (int tick = 0; tick < 2600; ++tick) {
                ElitePhase before = carrier->phase;
                float timeBefore = carrier->time;
                game->playerHitCooldown = 1;
                updateElite(game, 3);
                if (carrier->phase == ELITE_WARNING) {
                    attacks |= 1u << carrier->attack;
                    assert(carrier->duration >= 0.5f);
                }
                if (before == ELITE_WARNING && timeBefore <= STEP && carrier->attack == ATTACK_RING) ++emittedRings;
                // Simulate bolts leaving the arena between volleys.
                for (int j = 0; j < MAX_SHOTS; ++j) game->shots[j].life -= STEP;
            }
            assert(attacks & (1u << ATTACK_PINCER));
            assert(attacks & (1u << ATTACK_LANCE));
            assert(attacks & (1u << ATTACK_LANES));
            if (phase) assert(emittedRings >= 2);
            else {
                assert(attacks & (1u << ATTACK_FAN));
                damageElite(game, 3, carrier->maxLives);
            }
        }
    }
    puts("PASS: 20,800 encounter ticks, all five patterns, echo volley and warning floors");
}

static void snapshot(Game *game, RenderTexture2D world, State state, const char *file) {
    game->currentState = state;
    game->screenAge = 1;
    BeginDrawing();
    renderFrame(game, world, screenViewport());
    rlDrawRenderBatchActive();
    Image img = LoadImageFromScreen();
    assert(IsImageValid(img) && img.width == GetRenderWidth() && img.height == GetRenderHeight());
    assert(ExportImage(img, file));
    UnloadImage(img);
    EndDrawing();
}

static void resizeWindow(int width, int height) {
    SetWindowSize(width, height);
    // GLFW reports framebuffer changes through the event pump.
    for (int i = 0; i < 12; ++i) {
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
        if (GetRenderWidth() == width && GetRenderHeight() == height &&
            GetScreenWidth() == width && GetScreenHeight() == height) return;
        WaitTime(0.015);
    }
    assert(!"Framebuffer resize timed out");
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1200,720,"Flight verification");
    SetTraceLogLevel(LOG_WARNING);
    InitAudioDevice();
    SetRandomSeed(37);
    Game game;
    initializeGame(&game);
    loadSettings(&game);
    assert(IsTextureValid(game.sky) && IsTextureValid(game.carrierTexture));
    assert(game.bodyFont.texture.id != GetFontDefault().texture.id);
    testFlight(&game);
    testTimingAndMovement(&game);
    testProjectilePool(&game);
    testPatterns(&game);
    testSeededWorlds(&game);
    game.settings = DEFAULT_SETTINGS;
    game.settings.timerEnabled = true;
    resetRun(&game);
    strcpy(game.playerName, "ASTER");
    RenderTexture2D world = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTextureFilter(world.texture, TEXTURE_FILTER_BILINEAR);
    MakeDirectory("screenshots");
    snapshot(&game, world, STATE_MENU, "screenshots/menu.png");
    snapshot(&game, world, STATE_SETTING, "screenshots/settings.png");
    snapshot(&game, world, STATE_NAME_ENTRY, "screenshots/briefing.png");
    snapshot(&game, world, STATE_GAME, "screenshots/beacon.png");
    acquireFixes(&game);
    for (int i = 0; i < 3; ++i) damageElite(&game, i, 200);
    game.player.position = Vector2Add(game.mission.transport, (Vector2){150,120});
    game.player.previous = game.camera.target = game.previousCameraTarget = game.player.position;
    snapshot(&game, world, STATE_GAME, "screenshots/escort.png");
    escort(&game);
    game.player.previous = game.camera.target = game.previousCameraTarget = game.player.position;
    Elite *carrier = &game.elites[3];
    placeElite(&game, 3, Vector2Add(game.player.position, (Vector2){250,-100}));
    carrier->phase = ELITE_WARNING;
    carrier->attack = ATTACK_RING;
    carrier->aim = 2.75f;
    carrier->time = 0.25f;
    carrier->duration = 0.8f;
    game.elapsedTime = 82.5;
    game.dispatchTime = 0;
    snapshot(&game, world, STATE_GAME, "screenshots/carrier.png");
    snapshot(&game, world, STATE_PAUSE, "screenshots/paused.png");
    game.timedOut = true;
    game.elapsedTime = game.settings.timerMinutes * 60;
    snapshot(&game, world, STATE_GAME_OVER, "screenshots/debrief.png");
    snapshot(&game, world, STATE_LEADERBOARD, "screenshots/records.png");
    resizeWindow(640, 480);
    snapshot(&game, world, STATE_SETTING, "screenshots/settings-640.png");
    resizeWindow(1600, 700);
    snapshot(&game, world, STATE_MENU, "screenshots/menu-wide.png");
    UnloadRenderTexture(world);
    shutdownGame(&game);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
