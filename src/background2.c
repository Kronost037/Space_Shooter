#include "background.h"
#define STAR_COUNT 200
#define DUST_COUNT 150

#define NEBULA_COUNT 2
#define STREAK_COUNT 35
#define STREAK_DURATION 5.0f
#define PLANET_COUNT 3


typedef struct star
{
    Vector2 pos;
    Vector2 vel;
    float size;
    float brightness;
    float twinklespeed;
    float phase;
} star;

typedef struct Dust
{
    Vector2 position;
    float speed;
    float size;
    float alpha;
    float depth;
} Dust;
typedef struct Streak
{
    Vector2 pos;
    Vector2 vel;
    float length;
    float thickness;
    float alpha;
} Streak;

typedef struct nebula
{
    Vector2 pos;
    Vector2 vel;
    float size;
    float rotate;
    float alpha;
    Color tint;
} nebula;

typedef struct planet
{
    Vector2 pos;
    Vector2 vel;

    float size;

    float rotation;
    float rotateSpeed;

    float alpha;
} planet;

static Texture2D bgTexture;
static bool loaded = false;
static star stars[STAR_COUNT];
static bool starsInitialized = false;
static Dust dust[DUST_COUNT];
static bool dustInitialized = false;


static nebula nebulas[NEBULA_COUNT];
static bool nebulaInitialized = false;
static Streak streaks[STREAK_COUNT];

static Texture2D nebulaTexture;
static bool nebulaLoaded = false;
static bool streakInitialized = false;
static planet planets[PLANET_COUNT];

static bool planetsInitialized = false;
static Texture2D planetTexture;
static bool planetLoaded = false;


void InitStreaks(size_t width, size_t height)
{
    for (int i = 0; i < STREAK_COUNT; i++)
    {
        streaks[i].pos.x = GetRandomValue(width, width + 800);
        streaks[i].pos.y = GetRandomValue(-200, height);

        streaks[i].vel.x = -600;
        streaks[i].vel.y = 600;

        streaks[i].length = GetRandomValue(80, 250);
        streaks[i].thickness = GetRandomValue(2, 5);

        streaks[i].alpha = GetRandomValue(60, 100) / 100.0f;
    }
}

//init planets
void InitPlanets(size_t width, size_t height)
{
    for (int i = 0; i < PLANET_COUNT; i++)
    {
        // Left অথবা Right side
     planets[i].size = GetRandomValue(120, 200);

if (GetRandomValue(0,1))
{
    planets[i].pos.x =
        -planets[i].size / 2.0f;
}
else
{
    planets[i].pos.x =
        width - planets[i].size / 2.0f;
}

    if (i == 0)
{
    planets[i].pos.y = GetRandomValue(-300, 100);
}
else
{
    planets[i].pos.y = GetRandomValue(height/2, height-100);
}

        planets[i].vel.x = 0;
        planets[i].vel.y = GetRandomValue(5, 10);

        planets[i].rotation = GetRandomValue(0, 360);

        planets[i].rotateSpeed =
            (float)GetRandomValue(-2, 2);

        if (planets[i].rotateSpeed == 0)
            planets[i].rotateSpeed = 1;

        planets[i].alpha =
            GetRandomValue(20, 50) / 100.0f;
           \
    }
}
void InitNebula(size_t width, size_t height)
{
    for (int i = 0; i < NEBULA_COUNT; i++)
    {
        nebulas[i].pos.x = GetRandomValue(0, width);
        nebulas[i].pos.y = GetRandomValue(0, height);

        nebulas[i].vel.x = 0;
        nebulas[i].vel.y = GetRandomValue(5, 15);

        nebulas[i].size = GetRandomValue(250, 500);

        nebulas[i].rotate = GetRandomValue(0, 360);

        nebulas[i].alpha = GetRandomValue(10, 20) / 100.0f;
        int r = GetRandomValue(0, 2);

switch (r)
{
case 0:
    nebulas[i].tint = SKYBLUE;
    break;

case 1:
    nebulas[i].tint = VIOLET;
    break;

case 2:
    nebulas[i].tint = PINK;
    break;
}
    }
}

void InitDust(size_t width, size_t height)
{
    for (int i = 0; i < DUST_COUNT; i++)
    {
        dust[i].position.x = GetRandomValue(0, width);
        dust[i].position.y = GetRandomValue(0, height);

        dust[i].speed = GetRandomValue(10, 40);

        dust[i].size = (float)GetRandomValue(1, 2);

        dust[i].alpha = GetRandomValue(20, 80) / 100.0f;

        dust[i].depth = GetRandomValue(1, 3);
    }
}
void InitStars(size_t width, size_t height)
{
    for (int i = 0; i < STAR_COUNT; i++)
    {
        stars[i].pos.x = GetRandomValue(0, width);
        stars[i].pos.y = GetRandomValue(0, height);

        stars[i].size = GetRandomValue(0.5,1.5 );

        stars[i].brightness = GetRandomValue(100, 255);

        stars[i].twinklespeed = (float)GetRandomValue(1, 5) / 10.0f;

        stars[i].phase = (float)GetRandomValue(0, 360);

        stars[i].vel.x = 0;
stars[i].vel.y = GetRandomValue(50, 100);
    }
}

void drawBackground(size_t width, size_t height, float timer)
{

    (void)timer;
    if (!starsInitialized)
{
    InitStars(width, height);
    starsInitialized = true;
}

//planet init
if(!planetsInitialized)
{
    InitPlanets(width,height);

    planetsInitialized = true;
}
if (!streakInitialized)
{
    InitStreaks(width, height);
    streakInitialized = true;
}
if (!nebulaLoaded)
{
    nebulaTexture = LoadTexture("src/Assets/nev.png");
    nebulaLoaded = true;
}
if (!nebulaInitialized)
{
    InitNebula(width, height);
    nebulaInitialized = true;
}
//planet texture load
if (!planetLoaded)
{
    planetTexture = LoadTexture("src/Assets/p.png");
    planetLoaded = true;
}
if (!dustInitialized)
{
    InitDust(width, height);
    dustInitialized = true;
}
    if (!loaded)
    {
        bgTexture = LoadTexture("src/Assets/bg.png");

        TraceLog(
            LOG_INFO,
            "Texture: %d x %d",
            bgTexture.width,
            bgTexture.height
        );

        loaded = true;
    }

    ClearBackground(BLACK);

    if (bgTexture.id > 0)
    {
        Rectangle source = {
            0,
            0,
            (float)bgTexture.width,
            (float)bgTexture.height
        };

        Rectangle dest = {
            0,
            0,
            (float)width,
            (float)height
        };

        DrawTexturePro(
            bgTexture,
            source,
            dest,
            (Vector2){0, 0},
            0.0f,
            WHITE
        );
    }
    float dt = GetFrameTime();
if(timer<STREAK_DURATION){
        for (int i = 0; i < STREAK_COUNT; i++)
{
    streaks[i].pos.x += streaks[i].vel.x * dt;
    streaks[i].pos.y += streaks[i].vel.y * dt;

    if (streaks[i].pos.x < -streaks[i].length ||
        streaks[i].pos.y > height + streaks[i].length)
    {
        streaks[i].pos.x = GetRandomValue(width, width + 400);
        streaks[i].pos.y = GetRandomValue(-200, 0);
    }
}
}
    for (int i = 0; i < NEBULA_COUNT; i++)
{
    nebulas[i].pos.y += nebulas[i].vel.y * dt;

    nebulas[i].rotate += 2 * dt;

    if (nebulas[i].pos.y > height + nebulas[i].size)
    {
        nebulas[i].pos.y = -nebulas[i].size;
        nebulas[i].pos.x = GetRandomValue(0, width);
    }
}

for (int i = 0; i < DUST_COUNT; i++)
{
    dust[i].position.y += dust[i].speed * dt;

    if (dust[i].position.y > height)
    {
        dust[i].position.y = 0;
        dust[i].position.x = GetRandomValue(0, width);
    }
}

    for (int i = 0; i < STAR_COUNT; i++)
    {
        stars[i].pos.y += stars[i].vel.y * dt;

        if (stars[i].pos.y > height)
        {
            stars[i].pos.y = 0;
            stars[i].pos.x = GetRandomValue(0, (int)width);
        }
    }
    for (int i = 0; i < DUST_COUNT; i++)
{
    DrawCircleV(
        dust[i].position,
        dust[i].size,
        Fade(LIGHTGRAY, dust[i].alpha)
    );
}
for (int i = 0; i < NEBULA_COUNT; i++)
{
    Rectangle source =
    {
        0,
        0,
        nebulaTexture.width,
        nebulaTexture.height
    };

    Rectangle dest =
    {
        nebulas[i].pos.x,
        nebulas[i].pos.y,
        nebulas[i].size,
        nebulas[i].size
    };

    DrawTexturePro(
        nebulaTexture,
        source,
        dest,
        (Vector2){nebulas[i].size/2, nebulas[i].size/2},
        nebulas[i].rotate,
        Fade(WHITE, nebulas[i].alpha)
    );
}
//planet update
 for (int i = 0; i < PLANET_COUNT; i++)
{
    planets[i].pos.y += planets[i].vel.y * dt;

    planets[i].rotation +=
        planets[i].rotateSpeed * dt;
   if (planets[i].pos.y > height + planets[i].size)
{
    planets[i].pos.y = -planets[i].size;

    if (GetRandomValue(0,1))
        planets[i].pos.x = -planets[i].size/2.0f;
    else
        planets[i].pos.x = width - planets[i].size/2.0f;
}
}

if(timer<STREAK_DURATION){
    for (int i = 0; i < STREAK_COUNT; i++)
{
    Vector2 start = streaks[i].pos;

    Vector2 end =
    {
        start.x + streaks[i].length,
        start.y - streaks[i].length
    };

    DrawLineEx(
        start,
        end,
        streaks[i].thickness,
        Fade(WHITE, streaks[i].alpha)
    );

    DrawCircleV(
        start,
        streaks[i].thickness * 1.5f,
        Fade(WHITE, streaks[i].alpha)
    );
}
}
    // Draw planets

for (int i = 0; i < PLANET_COUNT; i++)
{
    Rectangle source =
    {
        0,
        0,
        (float)planetTexture.width,
        (float)planetTexture.height
    };

Rectangle dest =
{
    planets[i].pos.x,
    planets[i].pos.y,
    planets[i].size,
    planets[i].size
};

    DrawTexturePro(
        planetTexture,
        source,
        dest,
       (Vector2)
{
    planets[i].size/2.0f,
    planets[i].size/2.0f
},
        planets[i].rotation,
        Fade(WHITE, planets[i].alpha)
    );
}


    for (int i = 0; i < STAR_COUNT; i++)
    {
        float alpha =
    0.65f + 0.35f *
    sinf(timer * stars[i].twinklespeed + stars[i].phase);
    Vector2 p = stars[i].pos;
float glow = stars[i].size * 4;
DrawLineEx(
    (Vector2){p.x - glow, p.y},
    (Vector2){p.x + glow, p.y},
    stars[i].size * 0.9f,
    Fade(WHITE, alpha * 0.35f)
);
DrawLineEx(
    (Vector2){p.x, p.y - glow},
    (Vector2){p.x, p.y + glow},
    stars[i].size * 0.9f,
    Fade(WHITE, alpha * 0.35f)
);     
       DrawCircleV(
    p,
    stars[i].size*1.5f,
    Fade(WHITE, alpha)
);
        
    }
}


