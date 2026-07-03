#include "core.h"
#include <math.h>
#include <stdbool.h>

#ifndef TAU
#define TAU 6.28318530718f
#endif

#define BG_STAR_COUNT    132
#define BG_DUST_COUNT    34
#define BG_NEBULA_COUNT  3
#define BG_SHOT_COUNT    64

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float size;
    float phase;
    int layer;
} BgStar;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float size;
    float phase;
    float seed;
} BgDust;

typedef struct {
    bool active;
    Vector2 pos;
    Vector2 drift;
    float radius;
    float age;
    float life;
    float seed;
    Color base;
} BgNebula;

typedef struct {
    bool active;
    Vector2 pos;
    Vector2 dir;
    float age;
    float life;
    float speed;
    float tail;
    float seed;
} BgShot;

typedef struct {
    bool active;
    Vector2 pos;
    float radius;
    float age;
    float life;
    float drift;
    bool ringed;
    int side;
    float ringTilt;
    Color body;
    Color shadow;
} BgPlanet;

static int bg_cycle_seed = 1;

static Vector2 bg_shower_dir = { -0.88f, 0.47f };
static Vector2 bg_shower_origin = { 0.0f, 0.0f };

static float bg_frac(float x) {
    return x - floorf(x);
}

static float bg_hash01(int n) {
    return bg_frac(sinf((float)n * 12.9898f + 78.233f) * 43758.5453f);
}

static float bg_rand(int n, float min, float max) {
    return min + (max - min) * bg_hash01(n);
}

static float bg_wrap(float x, float maxv) {
    float r = fmodf(x, maxv);
    if (r < 0.0f) r += maxv;
    return r;
}

static float bg_clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static float bg_smoothstep(float a, float b, float x) {
    float t = bg_clampf((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static Color bg_alpha(Color c, unsigned char a) {
    c.a = a;
    return c;
}

static Color bg_lerp_color(Color a, Color b, float t) {
    t = bg_clampf(t, 0.0f, 1.0f);
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static Vector2 bg_vadd(Vector2 a, Vector2 b) {
    return (Vector2){ a.x + b.x, a.y + b.y };
}

static Vector2 bg_vscale(Vector2 v, float s) {
    return (Vector2){ v.x * s, v.y * s };
}

static Vector2 bg_vnorm(Vector2 v) {
    float len = sqrtf(v.x * v.x + v.y * v.y);
    if (len < 0.0001f) return (Vector2){ 0.0f, 0.0f };
    return (Vector2){ v.x / len, v.y / len };
}

static Vector2 bg_rotate(Vector2 v, float ang) {
    float c = cosf(ang);
    float s = sinf(ang);
    return (Vector2){ v.x * c - v.y * s, v.x * s + v.y * c };
}

static void bg_glow(Vector2 p, float radius, Color c) {
    for (int i = 6; i >= 1; --i) {
        float t = (float)i / 6.0f;
        unsigned char a = (unsigned char)((float)c.a * t * t * 0.55f);
        DrawCircleV(p, radius * t, bg_alpha(c, a));
    }
}

static void bg_plus_star(Vector2 p, float size, float twinkle, Color core, Color glow) {
    float pulse = 0.60f + 0.40f * sinf(twinkle);
    float arm = size * (1.05f + 0.55f * pulse);
    float thick = 1.0f + 0.8f * pulse;

    bg_glow(p, size * 3.0f, bg_alpha(glow, 70));

    DrawLineEx(
        (Vector2){ p.x - arm, p.y },
        (Vector2){ p.x + arm, p.y },
        thick,
        bg_alpha(glow, (unsigned char)(95 + 65 * pulse))
    );
    DrawLineEx(
        (Vector2){ p.x, p.y - arm },
        (Vector2){ p.x, p.y + arm },
        thick,
        bg_alpha(glow, (unsigned char)(95 + 65 * pulse))
    );

    DrawLineEx(
        (Vector2){ p.x - arm * 0.55f, p.y - arm * 0.55f },
        (Vector2){ p.x + arm * 0.55f, p.y + arm * 0.55f },
        1.0f,
        bg_alpha(glow, (unsigned char)(35 + 35 * pulse))
    );
    DrawLineEx(
        (Vector2){ p.x - arm * 0.55f, p.y + arm * 0.55f },
        (Vector2){ p.x + arm * 0.55f, p.y - arm * 0.55f },
        1.0f,
        bg_alpha(glow, (unsigned char)(35 + 35 * pulse))
    );

    DrawCircleV(p, size * 0.45f, bg_alpha(core, (unsigned char)(180 + 55 * pulse)));
    DrawCircleV(p, size * 0.16f, WHITE);
}

static void bg_draw_ribbon(int width, int height, float timer) {
    Vector2 a = { -width * 0.08f, height * 0.82f };
    Vector2 b = { width * 0.50f, height * 0.56f };
    Vector2 c = { width * 1.08f, height * 0.18f };

    for (int i = 0; i <= 12; ++i) {
        float t = (float)i / 12.0f;

        Vector2 p0 = {
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        };
        Vector2 p1 = {
            b.x + (c.x - b.x) * t,
            b.y + (c.y - b.y) * t
        };

        Vector2 p = {
            p0.x + (p1.x - p0.x) * t + sinf(timer * 0.05f + (float)i) * 18.0f,
            p0.y + (p1.y - p0.y) * t + cosf(timer * 0.04f + (float)i * 1.4f) * 10.0f
        };

        float radius = width * (0.042f + 0.05f * sinf((float)i * 0.9f + 1.0f));
        float alphaPulse = 0.45f + 0.55f * sinf(timer * 0.03f + (float)i * 1.1f);

        Color bluish = (Color){ 70, 110, 180, 255 };
        Color purplish = (Color){ 110, 70, 175, 255 };
        Color soft = (Color){ 255, 255, 255, 255 };

        DrawCircleV(p, radius, bg_alpha((i % 2 == 0) ? bluish : purplish, (unsigned char)(6.0f + 7.0f * alphaPulse)));
        DrawCircleV(p, radius * 0.56f, bg_alpha(soft, (unsigned char)(2.0f + 3.0f * alphaPulse)));
    }
}

static void bg_draw_dust(BgDust *d, float width, float height, float dt, float timer) {
    d->pos.x += d->vel.x * dt;
    d->pos.y += d->vel.y * dt;

    d->pos.x = bg_wrap(d->pos.x, width);
    d->pos.y = bg_wrap(d->pos.y, height);

    float pulse = 0.55f + 0.45f * sinf(timer * 0.24f + d->phase);
    unsigned char a = (unsigned char)(14.0f + 24.0f * pulse);

    bg_glow(d->pos, d->size * 2.6f, bg_alpha((Color){ 150, 190, 255, 255 }, (unsigned char)(a * 0.35f)));
    DrawCircleV(d->pos, d->size, bg_alpha((Color){ 220, 230, 255, 255 }, a));
}

static void bg_draw_star(BgStar *s, float width, float height, float dt, float timer) {
    s->pos.x += s->vel.x * dt;
    s->pos.y += s->vel.y * dt;

    s->pos.x = bg_wrap(s->pos.x, width);
    s->pos.y = bg_wrap(s->pos.y, height);

    float tw = timer * (0.55f + 0.30f * (float)s->layer) + s->phase;
    float pulse = 0.58f + 0.42f * sinf(tw);

    float tint = bg_frac(s->phase * 0.137f + (float)s->layer * 0.41f);
    Color core = WHITE;
    Color glow;

    if (tint < 0.70f) {
        glow = (Color){ 165, 205, 255, 255 };
        if (s->layer == 0) core = (Color){ 220, 230, 255, 255 };
    } else if (tint < 0.88f) {
        glow = (Color){ 255, 232, 180, 255 };
        if (s->layer == 0) core = (Color){ 255, 246, 220, 255 };
    } else {
        glow = (Color){ 255, 180, 214, 255 };
        if (s->layer == 0) core = (Color){ 255, 238, 246, 255 };
    }

    if (s->layer == 0) {
        DrawCircleV(s->pos, s->size, bg_alpha(core, (unsigned char)(36 + 22 * pulse)));
        return;
    }

    if (s->layer == 1) {
        bg_plus_star(s->pos, s->size, tw, core, glow);
        return;
    }

    bg_plus_star(s->pos, s->size, tw, core, glow);
}

static void bg_draw_nebula(const BgNebula *n) {
    float t = n->age / n->life;
    float fadeIn = bg_smoothstep(0.0f, 0.20f, t);
    float fadeOut = 1.0f - bg_smoothstep(0.76f, 1.0f, t);
    float fade = bg_clampf(fadeIn * fadeOut, 0.0f, 1.0f);

    Vector2 c = {
        n->pos.x + n->drift.x * n->age,
        n->pos.y + n->drift.y * n->age
    };

    Color c1 = n->base;
    Color c2 = bg_lerp_color(n->base, WHITE, 0.16f);
    Color c3 = bg_lerp_color(n->base, (Color){ 24, 28, 72, 255 }, 0.28f);
    Color c4 = bg_lerp_color(n->base, (Color){ 70, 200, 255, 255 }, 0.12f);
    Color c5 = bg_lerp_color(n->base, (Color){ 255, 190, 120, 255 }, 0.10f);

    for (int i = 0; i < 12; ++i) {
        float a = n->seed + (float)i * 0.78f;
        float d = n->radius * bg_rand((int)(n->seed * 1000.0f) + i * 17, 0.18f, 1.0f);
        float rr = n->radius * bg_rand((int)(n->seed * 2000.0f) + i * 9, 0.13f, 0.34f);
        float wobble = 0.82f + 0.18f * sinf(n->age * 0.18f + (float)i);

        Vector2 p = {
            c.x + cosf(a) * d * wobble,
            c.y + sinf(a) * d * wobble
        };

        unsigned char alpha = (unsigned char)(bg_rand((int)(n->seed * 3000.0f) + i * 11, 8.0f, 24.0f) * fade);

        Color pick = c1;
        switch (i % 5) {
            case 1: pick = c2; break;
            case 2: pick = c3; break;
            case 3: pick = c4; break;
            case 4: pick = c5; break;
            default: break;
        }

        DrawCircleV(p, rr, bg_alpha(pick, alpha));
    }

    DrawCircleV(c, n->radius * 0.82f, bg_alpha(n->base, (unsigned char)(11.0f * fade)));
    DrawCircleV(c, n->radius * 0.55f, bg_alpha(WHITE, (unsigned char)(5.0f * fade)));
}

static void bg_draw_shot(const BgShot *s) {
    float t = s->age / s->life;
    float fadeIn = bg_smoothstep(0.0f, 0.08f, t);
    float fadeOut = 1.0f - bg_smoothstep(0.78f, 1.0f, t);
    float fade = bg_clampf(fadeIn * fadeOut, 0.0f, 1.0f);

    Vector2 head = {
        s->pos.x + s->dir.x * s->speed * s->age,
        s->pos.y + s->dir.y * s->speed * s->age
    };

    Vector2 tail = {
        head.x - s->dir.x * s->tail,
        head.y - s->dir.y * s->tail
    };

    float hue = bg_frac(s->seed * 0.173f);
    Color headCol;
    Color glowCol;

    if (hue < 0.65f) {
        headCol = (Color){ 255, 255, 255, 255 };
        glowCol = (Color){ 170, 210, 255, 255 };
    } else if (hue < 0.85f) {
        headCol = (Color){ 255, 248, 220, 255 };
        glowCol = (Color){ 255, 220, 150, 255 };
    } else {
        headCol = (Color){ 255, 235, 245, 255 };
        glowCol = (Color){ 255, 170, 205, 255 };
    }

    float flicker = 0.92f + 0.08f * sinf(s->seed * 6.0f + s->age * 40.0f);

    bg_glow(head, 12.0f * flicker, bg_alpha(glowCol, (unsigned char)(90.0f * fade * flicker)));

    for (int i = 0; i < 10; ++i) {
        float k = (float)i / 9.0f;
        float trailEase = 1.0f - k;
        Vector2 p = {
            tail.x + (head.x - tail.x) * k,
            tail.y + (head.y - tail.y) * k
        };

        unsigned char a = (unsigned char)(95.0f * fade * trailEase * trailEase);
        DrawCircleV(
            p,
            1.4f + trailEase * 2.2f,
            bg_alpha(bg_lerp_color(glowCol, headCol, 0.55f), a)
        );
    }

    DrawLineEx(tail, head, 4.0f, bg_alpha(glowCol, (unsigned char)(105.0f * fade)));
    DrawLineEx(
        (Vector2){ tail.x + s->dir.x * 18.0f, tail.y + s->dir.y * 18.0f },
        head,
        2.0f,
        bg_alpha(headCol, (unsigned char)(220.0f * fade))
    );
    DrawCircleV(head, 2.8f, bg_alpha(headCol, (unsigned char)(240.0f * fade)));
}

static void bg_draw_planet(const BgPlanet *p) {
    float t = p->age / p->life;
    float fadeIn = bg_smoothstep(0.0f, 0.16f, t);
    float fadeOut = 1.0f - bg_smoothstep(0.80f, 1.0f, t);
    float fade = bg_clampf(fadeIn * fadeOut, 0.0f, 1.0f);

    Vector2 c = p->pos;

    Color base = p->body;
    Color dark = p->shadow;
    Color bright = bg_lerp_color(p->body, WHITE, 0.16f);
    Color ringA = bg_lerp_color(p->body, WHITE, 0.12f);
    Color ringB = bg_lerp_color(p->shadow, p->body, 0.28f);
    Color halo = bg_lerp_color(p->body, (Color){ 80, 190, 255, 255 }, 0.12f);

    bg_glow(c, p->radius * 2.1f, bg_alpha(halo, (unsigned char)(26.0f * fade)));

    DrawCircleV(c, p->radius * 1.72f, bg_alpha(base, 18));
    DrawCircleV(c, p->radius, bg_alpha(base, (unsigned char)(255.0f * fade)));

    DrawCircleV(
        (Vector2){ c.x - p->radius * 0.26f, c.y - p->radius * 0.24f },
        p->radius * 0.74f,
        bg_alpha(bright, (unsigned char)(28.0f * fade))
    );

    DrawCircleV(
        (Vector2){ c.x + p->radius * 0.14f, c.y + p->radius * 0.14f },
        p->radius * 0.92f,
        bg_alpha(dark, (unsigned char)(76.0f * fade))
    );

    /* colored bands to make the sphere read as a real planet */
    DrawCircleV(
        (Vector2){ c.x - p->radius * 0.03f, c.y - p->radius * 0.08f },
        p->radius * 0.92f,
        bg_alpha(bg_lerp_color(base, (Color){ 255, 255, 255, 255 }, 0.08f), (unsigned char)(11.0f * fade))
    );
    DrawCircleV(
        (Vector2){ c.x + p->radius * 0.02f, c.y + p->radius * 0.08f },
        p->radius * 0.78f,
        bg_alpha(bg_lerp_color(base, dark, 0.22f), (unsigned char)(18.0f * fade))
    );
    DrawCircleV(
        (Vector2){ c.x - p->radius * 0.04f, c.y + p->radius * 0.20f },
        p->radius * 0.58f,
        bg_alpha(bg_lerp_color(base, dark, 0.36f), (unsigned char)(16.0f * fade))
    );

    float rx = p->radius * 2.10f;
    float ry = p->radius * (0.34f + 0.10f * p->ringTilt);

    DrawEllipseLines((int)c.x, (int)c.y, rx, ry, bg_alpha(ringA, (unsigned char)(20.0f * fade)));
    DrawEllipseLines((int)(c.x + 1), (int)(c.y + 1), rx * 0.98f, ry * 0.98f, bg_alpha(ringB, (unsigned char)(30.0f * fade)));

    DrawRing(c, p->radius * 1.02f, p->radius * 1.92f, 18.0f, 342.0f, 96, bg_alpha(ringA, (unsigned char)(14.0f * fade)));
    DrawRing(
        (Vector2){ c.x + p->radius * 0.02f, c.y + p->radius * 0.03f },
        p->radius * 0.98f,
        p->radius * 1.96f,
        22.0f,
        338.0f,
        96,
        bg_alpha(ringB, (unsigned char)(30.0f * fade))
    );

    DrawCircleLines((int)c.x, (int)c.y, p->radius, bg_alpha(WHITE, (unsigned char)(18.0f * fade)));
}

static void bg_spawn_nebula(BgNebula *n, int slot, int width, int height) {
    n->active = true;
    n->age = 0.0f;
    n->life = bg_rand(bg_cycle_seed * 100 + slot * 13, 22.0f, 34.0f);
    n->radius = bg_rand(bg_cycle_seed * 101 + slot * 17, 140.0f, 300.0f);
    n->seed = bg_rand(bg_cycle_seed * 102 + slot * 19, 0.0f, TAU);

    int palette = (bg_cycle_seed + slot) % 5;
    switch (palette) {
        case 0: n->base = (Color){ 120, 72, 205, 255 }; break;
        case 1: n->base = (Color){ 54, 160, 224, 255 }; break;
        case 2: n->base = (Color){ 196, 92, 138, 255 }; break;
        case 3: n->base = (Color){ 214, 148, 72, 255 }; break;
        default: n->base = (Color){ 98, 120, 230, 255 }; break;
    }

    int side = GetRandomValue(0, 3);
    if (side == 0) {
        n->pos = (Vector2){ -n->radius * 0.60f, bg_rand(bg_cycle_seed * 103 + slot * 23, 0.10f, 0.82f) * height };
        n->drift = (Vector2){ bg_rand(bg_cycle_seed * 104 + slot * 29, 1.0f, 4.0f), bg_rand(bg_cycle_seed * 105 + slot * 31, -0.40f, 0.40f) };
    } else if (side == 1) {
        n->pos = (Vector2){ width + n->radius * 0.60f, bg_rand(bg_cycle_seed * 106 + slot * 23, 0.10f, 0.82f) * height };
        n->drift = (Vector2){ bg_rand(bg_cycle_seed * 107 + slot * 29, -4.0f, -1.0f), bg_rand(bg_cycle_seed * 108 + slot * 31, -0.40f, 0.40f) };
    } else if (side == 2) {
        n->pos = (Vector2){ bg_rand(bg_cycle_seed * 109 + slot * 23, 0.05f, 0.95f) * width, -n->radius * 0.45f };
        n->drift = (Vector2){ bg_rand(bg_cycle_seed * 110 + slot * 29, -1.2f, 1.2f), bg_rand(bg_cycle_seed * 111 + slot * 31, 1.0f, 3.5f) };
    } else {
        n->pos = (Vector2){ bg_rand(bg_cycle_seed * 112 + slot * 23, 0.05f, 0.95f) * width, height + n->radius * 0.45f };
        n->drift = (Vector2){ bg_rand(bg_cycle_seed * 113 + slot * 29, -1.2f, 1.2f), bg_rand(bg_cycle_seed * 114 + slot * 31, -3.5f, -1.0f) };
    }
}

static void bg_spawn_shower(BgShot *shots, int count, int width, int height) {
    (void)height;
    
    Vector2 dir = bg_vnorm(bg_shower_dir);
    if (dir.x == 0.0f && dir.y == 0.0f) dir = (Vector2){ -0.88f, 0.47f };

    Vector2 perp = (Vector2){ -dir.y, dir.x };

    for (int i = 0; i < count; ++i) {
        for (int s = 0; s < BG_SHOT_COUNT; ++s) {
            if (shots[s].active) continue;

            float lane = bg_rand(bg_cycle_seed * 210 + s * 7 + i * 31, -0.52f * (float)width, 0.52f * (float)width);
            float startBack = bg_rand(bg_cycle_seed * 211 + s * 11 + i * 37, 160.0f, 340.0f);

            Vector2 start = bg_vadd(
                bg_shower_origin,
                bg_vadd(bg_vscale(perp, lane), bg_vscale(dir, -startBack))
            );

            shots[s].active = true;
            shots[s].pos = start;
            shots[s].dir = bg_rotate(dir, bg_rand(bg_cycle_seed * 212 + s * 13 + i * 41, -0.05f, 0.05f));
            shots[s].age = 0.0f;
            shots[s].life = bg_rand(bg_cycle_seed * 213 + s * 17 + i * 43, 2.2f, 4.4f);
            shots[s].speed = bg_rand(bg_cycle_seed * 214 + s * 19 + i * 47, 500.0f, 860.0f);
            shots[s].tail = bg_rand(bg_cycle_seed * 215 + s * 23 + i * 53, 260.0f, 460.0f);
            shots[s].seed = bg_rand(bg_cycle_seed * 216 + s * 29 + i * 59, 0.0f, TAU);
            break;
        }
    }
}

static void bg_spawn_planet(BgPlanet *p, int width, int height) {
    p->active = true;
    p->age = 0.0f;
    p->life = bg_rand(bg_cycle_seed * 300 + 1, 18.0f, 30.0f);
    p->radius = bg_rand(bg_cycle_seed * 301 + 2, 220.0f, 360.0f);
    p->drift = bg_rand(bg_cycle_seed * 302 + 3, 0.30f, 0.95f);
    p->ringed = true;
    p->ringTilt = bg_rand(bg_cycle_seed * 303 + 4, 0.18f, 0.92f);
    p->side = (GetRandomValue(0, 1) == 0) ? 0 : 1;

    if (p->side == 0) {
        p->pos.x = -p->radius * bg_rand(bg_cycle_seed * 304 + 5, 0.78f, 0.98f);
    } else {
        p->pos.x = width + p->radius * bg_rand(bg_cycle_seed * 305 + 6, 0.78f, 0.98f);
    }

    p->pos.y = bg_rand(bg_cycle_seed * 306 + 7, 0.18f, 0.72f) * height;

    switch ((bg_cycle_seed + GetRandomValue(0, 5)) % 5) {
        case 0:
            p->body = (Color){ 116, 94, 210, 255 };
            p->shadow = (Color){ 42, 28, 72, 255 };
            break;
        case 1:
            p->body = (Color){ 72, 156, 196, 255 };
            p->shadow = (Color){ 24, 54, 62, 255 };
            break;
        case 2:
            p->body = (Color){ 196, 132, 78, 255 };
            p->shadow = (Color){ 72, 40, 20, 255 };
            break;
        case 3:
            p->body = (Color){ 176, 84, 132, 255 };
            p->shadow = (Color){ 58, 24, 46, 255 };
            break;
        default:
            p->body = (Color){ 92, 170, 112, 255 };
            p->shadow = (Color){ 30, 56, 34, 255 };
            break;
    }
}

void drawBackground(int width, int height, float timer) {
    static bool initialized = false;

    static BgStar stars[BG_STAR_COUNT];
    static BgDust dust[BG_DUST_COUNT];
    static BgNebula nebula[BG_NEBULA_COUNT];
    static BgShot shots[BG_SHOT_COUNT];
    static BgPlanet planet;

    static float nebula_rest = 18.0f;
    static float nebula_active = 0.0f;
    static float shot_rest = 8.0f;
    static float shot_active = 0.0f;
    static float shot_spawner = 0.0f;
    static float planet_rest = 120.0f;

    if (!initialized) {
        initialized = true;

        for (int i = 0; i < BG_STAR_COUNT; ++i) {
            stars[i].layer = (i < 72) ? 0 : (i < 112) ? 1 : 2;
            stars[i].pos = (Vector2){
                bg_rand(100 + i * 3, 0.0f, (float)width),
                bg_rand(200 + i * 5, 0.0f, (float)height)
            };
            stars[i].size = (stars[i].layer == 0) ? bg_rand(300 + i, 0.6f, 1.15f)
                           : (stars[i].layer == 1) ? bg_rand(300 + i, 0.95f, 1.55f)
                                                  : bg_rand(300 + i, 1.35f, 2.0f);
            stars[i].phase = bg_rand(400 + i, 0.0f, TAU);
            stars[i].vel = (Vector2){
                bg_rand(500 + i, -2.0f, 2.0f) * ((stars[i].layer == 0) ? 0.7f : (stars[i].layer == 1) ? 1.3f : 2.3f),
                bg_rand(600 + i, -1.2f, 1.2f) * ((stars[i].layer == 0) ? 0.6f : (stars[i].layer == 1) ? 1.1f : 1.8f)
            };
        }

        for (int i = 0; i < BG_DUST_COUNT; ++i) {
            dust[i].pos = (Vector2){
                bg_rand(700 + i * 7, 0.0f, (float)width),
                bg_rand(800 + i * 9, 0.0f, (float)height)
            };
            dust[i].vel = (Vector2){
                bg_rand(900 + i * 11, -10.0f, 10.0f),
                bg_rand(1000 + i * 13, -6.0f, 6.0f)
            };
            dust[i].size = bg_rand(1100 + i * 17, 0.75f, 1.9f);
            dust[i].phase = bg_rand(1200 + i * 19, 0.0f, TAU);
            dust[i].seed = bg_rand(1300 + i * 23, 0.0f, TAU);
        }

        for (int i = 0; i < BG_NEBULA_COUNT; ++i) nebula[i].active = false;
        for (int i = 0; i < BG_SHOT_COUNT; ++i) shots[i].active = false;
        planet.active = false;
    }

    float dt = GetFrameTime();

    DrawRectangleGradientV(
        0, 0, width, height,
        (Color){ 4, 7, 18, 255 },
        (Color){ 9, 13, 28, 255 }
    );

    DrawCircleV(
        (Vector2){
            width * 0.18f + sinf(timer * 0.05f) * 36.0f,
            height * 0.20f + cosf(timer * 0.04f) * 18.0f
        },
        width * 0.58f,
        bg_alpha((Color){ 28, 18, 50, 255 }, 26)
    );
    DrawCircleV(
        (Vector2){
            width * 0.84f + cosf(timer * 0.03f) * 24.0f,
            height * 0.20f + sinf(timer * 0.05f) * 14.0f
        },
        width * 0.50f,
        bg_alpha((Color){ 12, 28, 52, 255 }, 18)
    );

    bg_draw_ribbon(width, height, timer);

    for (int i = 0; i < BG_STAR_COUNT; ++i) {
        bg_draw_star(&stars[i], (float)width, (float)height, dt, timer);
    }

    for (int i = 0; i < BG_DUST_COUNT; ++i) {
        bg_draw_dust(&dust[i], (float)width, (float)height, dt, timer);
    }

    nebula_rest -= dt;
    if (nebula_rest <= 0.0f && nebula_active <= 0.0f) {
        int showCount = GetRandomValue(1, 2);
        for (int i = 0; i < showCount; ++i) {
            for (int slot = 0; slot < BG_NEBULA_COUNT; ++slot) {
                if (!nebula[slot].active) {
                    bg_spawn_nebula(&nebula[slot], slot, width, height);
                    nebula_active = nebula[slot].life;
                    break;
                }
            }
        }

        nebula_rest = bg_rand(bg_cycle_seed * 120 + 1, 90.0f, 160.0f);
        bg_cycle_seed++;
    }

    if (nebula_active > 0.0f) nebula_active -= dt;

    for (int i = 0; i < BG_NEBULA_COUNT; ++i) {
        if (!nebula[i].active) continue;
        nebula[i].age += dt;
        if (nebula[i].age >= nebula[i].life) {
            nebula[i].active = false;
        } else {
            bg_draw_nebula(&nebula[i]);
        }
    }

    shot_rest -= dt;
    if (shot_rest <= 0.0f && shot_active <= 0.0f) {
        shot_active = bg_rand(bg_cycle_seed * 220 + 1, 8.0f, 14.0f);
        shot_rest = bg_rand(bg_cycle_seed * 221 + 2, 55.0f, 110.0f);
        shot_spawner = 0.0f;

        float ang = bg_rand(bg_cycle_seed * 222 + 3, 0.22f, 0.55f);
        if (GetRandomValue(0, 1) == 0) ang = -ang;

        bg_shower_dir = bg_vnorm((Vector2){
            (GetRandomValue(0, 1) == 0 ? -1.0f : 1.0f) * cosf(ang),
            sinf(fabsf(ang)) + 0.24f
        });

        if (bg_shower_dir.x == 0.0f && bg_shower_dir.y == 0.0f) {
            bg_shower_dir = (Vector2){ -0.88f, 0.47f };
        }

        Vector2 perp = (Vector2){ -bg_shower_dir.y, bg_shower_dir.x };
        float lane = bg_rand(bg_cycle_seed * 223 + 4, -0.16f * (float)height, 0.16f * (float)height);
        float back = bg_rand(bg_cycle_seed * 224 + 5, 160.0f, 280.0f);

        bg_shower_origin = bg_vadd(
            (Vector2){
                (bg_shower_dir.x < 0.0f) ? (float)width + 0.12f * (float)width : -0.12f * (float)width,
                bg_rand(bg_cycle_seed * 225 + 6, -0.06f * (float)height, 0.12f * (float)height)
            },
            bg_vadd(bg_vscale(perp, lane), bg_vscale(bg_shower_dir, -back))
        );

        bg_cycle_seed++;
    }

    if (shot_active > 0.0f) {
        shot_active -= dt;
        shot_spawner += dt;
    }

    for (int i = 0; i < BG_SHOT_COUNT; ++i) {
        if (!shots[i].active) continue;
        shots[i].age += dt;
        if (shots[i].age >= shots[i].life) {
            shots[i].active = false;
        } else {
            bg_draw_shot(&shots[i]);
        }
    }

    if (shot_active > 0.0f && shot_spawner >= 0.08f) {
        shot_spawner = 0.0f;
        int batch = GetRandomValue(3, 6);
        bg_spawn_shower(shots, batch, width, height);
    }

    planet_rest -= dt;
    if (!planet.active && planet_rest <= 0.0f) {
        bg_spawn_planet(&planet, width, height);
        planet_rest = bg_rand(bg_cycle_seed * 320 + 1, 180.0f, 360.0f);
        bg_cycle_seed++;
    }

    if (planet.active) {
        planet.age += dt;
        planet.pos.x += (planet.side == 0 ? 1.0f : -1.0f) * planet.drift * dt;

        if (planet.age >= planet.life) {
            planet.active = false;
        } else {
            bg_draw_planet(&planet);
        }
    }

    DrawRectangle(0, 0, width, 90, bg_alpha((Color){ 0, 0, 0, 255 }, 22));
    DrawRectangle(0, height - 110, width, 110, bg_alpha((Color){ 0, 0, 0, 255 }, 30));
}
