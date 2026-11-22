// Exercise the production fixed-step loop and the native window renderer under Xvfb.
#define main spaceShooterMain
#include "../src/core.c"
#undef main
#include "../src/game.c"
#include <assert.h>
#include <rlgl.h>

static void placeElite(Game *game, int index, Vector2 at) {
    Elite *elite = &game->elites[index];
    elite->ship.previous = elite->ship.position = at;
}

static void contact(Game *game, int owner, Vector2 at) {
    assert(addShot(game, owner, at, (Vector2){1,0}, 1, 4, 1));
    updateShots(game);
}

static void testFlight(Game *game) {
    for (int d = EASY; d <= INSANE; ++d) {
        game->settings.difficulty = (Difficulty)d;
        resetRun(game);
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
        updateScouts(game);
        int escorts = 0;
        for (int i = 0; i < MAX_ENEMIES; ++i) escorts += game->scouts[i].lives;
        assert(escorts == 1 && game->scoutKills == 0 && game->score == 0);

        game->settings.timerEnabled = true;
        game->settings.timerMinutes = 1;
        game->flightTicks = 7199;
        step(game, (Vector2){0}, (Vector2){0}, false);
        assert(game->timedOut && game->elapsedTime == 60 && !game->missionCleared);
        game->settings.timerEnabled = false;
        resetRun(game);
        assert(!game->timedOut && game->flightTicks == 0 && game->score == 0);
        for (int i = 0; i < 3; ++i) damageElite(game, i, 200);
        assert(game->elites[3].unlocked && game->wardensCleared == 3);
        damageElite(game, 3, 200);
        assert(game->elites[3].ship.lives == game->elites[3].maxLives / 2);
        damageElite(game, 3, 200);
        assert(game->missionCleared);
    }
    puts("PASS: four difficulties, visible-edge hits, HUD and lock gates, armor, escort cap, timer, reset and clear");
}

static void snapshot(Game *game, RenderTexture2D world, State state, const char *file) {
    game->currentState = state;
    BeginDrawing();
    renderFrame(game, world, screenViewport());
    rlDrawRenderBatchActive();
    Image img = LoadImageFromScreen();
    assert(IsImageValid(img));
    assert(ExportImage(img, file));
    UnloadImage(img);
    EndDrawing();
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
    for (int i = 0; i < 3; ++i) damageElite(&game, i, 200);
    Elite *carrier = &game.elites[3];
    placeElite(&game, 3, Vector2Add(game.player.position, (Vector2){250,-100}));
    carrier->phase = ELITE_WARNING;
    carrier->attack = ATTACK_RING;
    carrier->aim = 2.75f;
    carrier->time = 0.25f;
    carrier->duration = 0.8f;
    snapshot(&game, world, STATE_GAME, "screenshots/carrier.png");
    snapshot(&game, world, STATE_PAUSE, "screenshots/paused.png");
    game.timedOut = true;
    snapshot(&game, world, STATE_GAME_OVER, "screenshots/debrief.png");
    snapshot(&game, world, STATE_LEADERBOARD, "screenshots/records.png");
    SetWindowSize(640, 480);
    snapshot(&game, world, STATE_SETTING, "screenshots/settings-640.png");
    SetWindowSize(1600, 700);
    snapshot(&game, world, STATE_MENU, "screenshots/menu-wide.png");
    UnloadRenderTexture(world);
    unloadGameAssets(&game);
    UnloadMusicStream(game.menu->bg_song);
    UnloadSound(game.menu->hover_sfx);
    UnloadSound(game.menu->click_sfx);
    free(game.menu);
    leaderboardShutdown(&game.leaderboard);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
