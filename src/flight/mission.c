#include "mission.h"
#include "flight.h"

static const Color beacon = {134,223,207,255};
static const float cargoLoadDuration = 1.2f;

void startMission(Game *game, Vector2 launch) {
    game->mission = (Mission){0};
    game->mission.stage = MISSION_SURVEY;
    game->mission.waypoint = Vector2Add(launch, (Vector2){-190,-100});
}

void beginEscort(Game *game, Vector2 relay) {
    Mission *m = &game->mission;
    m->stage = MISSION_ESCORT;
    m->transport = relay;
    Vector2 route = Vector2Normalize(Vector2Subtract(game->elites[MAX_ELITES - 1].ship.position, relay));
    Vector2 side = {-route.y, route.x};
    m->escortRoute[0] = Vector2Add(relay, Vector2Add(Vector2Scale(route, 360), Vector2Scale(side, 215)));
    m->escortRoute[1] = Vector2Subtract(game->elites[MAX_ELITES - 1].ship.position, Vector2Scale(route, 250));
    m->waypoint = m->escortRoute[0];
    game->dispatch = "Mercy Six: stay close.";
    game->dispatchTime = 4;
}

void advanceMission(Game *game, float dt) {
    Mission *m = &game->mission;
    if (m->stage == MISSION_SURVEY && Vector2Distance(game->player.position, m->waypoint) < 72) {
        burst(game, m->waypoint, beacon, 14);
        game->payoutPosition = m->waypoint;
        award(game, 15);
        ++m->fixes;
        if (m->fixes == 1) {
            game->dispatch = "Mercy Six: Signal received. Following your lights.";
            game->dispatchTime = 3.0f;
        }
        if (m->fixes < SURVEY_GATES) {
            static const Vector2 route[] = {{-820,-490}, {620,-670}};
            m->waypoint = Vector2Add(m->waypoint, route[m->fixes - 1]);
        } else {
            m->stage = MISSION_RELAYS;
            for (int i = 0; i < MAX_ELITES - 1; ++i) game->elites[i].unlocked = true;
            game->dispatch = "Aster Lead: Wardens on radar.";
            game->dispatchTime = 3.8f;
        }
    } else if (m->stage == MISSION_ESCORT) {
        m->contested = false;
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            if (game->scouts[i].lives && Vector2Distance(m->transport, game->scouts[i].position) < 115) {
                m->contested = true;
                break;
            }
        }
        if (m->contested || Vector2Distance(game->player.position, m->transport) > 290) return;
        m->transport = Vector2MoveTowards(m->transport, m->waypoint, 145 * dt);
        if (Vector2Distance(m->transport, m->waypoint) > 1) return;
        if (m->escortLeg == 0) {
            m->loadingTime = fminf(m->loadingTime + dt, cargoLoadDuration);
            if (m->loadingTime < cargoLoadDuration) return;
        }
        burst(game, m->transport, beacon, 32);
        if (++m->escortLeg < ESCORT_LEGS) {
            m->waypoint = m->escortRoute[m->escortLeg];
            game->dispatch = "Mercy Six: medicine aboard.";
            game->dispatchTime = 2.6f;
            return;
        }
        game->payoutPosition = m->transport;
        award(game, 90);
        game->elites[MAX_ELITES - 1].unlocked = true;
        m->stage = MISSION_CARRIER;
        game->dispatch = "Mercy Six: cargo delivered.";
        game->dispatchTime = 3.8f;
    }
}

const char *missionTitle(const Game *game) {
    switch (game->mission.stage) {
    case MISSION_SURVEY: return TextFormat("Beacon %d / %d", game->mission.fixes + 1, SURVEY_GATES);
    case MISSION_RELAYS: return TextFormat("Wardens %d / 3", game->wardensCleared);
    case MISSION_ESCORT:
        if (game->mission.contested) return "Clear the route";
        if (Vector2Distance(game->player.position, game->mission.transport) > 290) return "Stay near Mercy Six";
        if (game->mission.escortLeg == 0) return game->mission.loadingTime > 0 ? "Loading medicine" : "Escort to pickup";
        return "Deliver medicine";
    case MISSION_CARRIER: return "Destroy the Carrier";
    }
    return "";
}

bool missionWaypoint(const Game *game, Vector2 *position) {
    if (game->mission.stage == MISSION_SURVEY) *position = game->mission.waypoint;
    else if (game->mission.stage == MISSION_ESCORT) *position = game->mission.transport;
    else return false;
    return true;
}

void drawTransport(const Game *game, Vector2 center, Vector2 heading, float length, float thrust) {
    Vector2 side = {-heading.y, heading.x};
    for (int engine = -1; engine <= 1; engine += 2) {
        Vector2 nozzle = Vector2Add(Vector2Subtract(center, Vector2Scale(heading, length * 0.465f)),
                                   Vector2Scale(side, engine * length * 0.042f));
        Vector2 tip = Vector2Subtract(nozzle, Vector2Scale(heading, length * (0.01f + thrust * 0.08f)));
        DrawLineEx(nozzle, tip, length * 0.022f, Fade(beacon, 0.32f));
        DrawCircleGradient(nozzle, length * 0.035f, Fade(beacon, 0.65f), Fade(beacon, 0));
    }
    drawShip(game->transportTexture, center, heading, length, WHITE, beacon);
}

void drawMission(const Game *game) {
    const Mission *m = &game->mission;
    if (m->stage != MISSION_SURVEY && m->stage != MISSION_ESCORT) return;
    float t = (float)game->elapsedTime;
    if (m->stage == MISSION_SURVEY) {
        // Three inward-facing transmitter heads leave the flight path open.
        for (int i = 0; i < 3; ++i) {
            Vector2 radial = direction(-PI / 2 + i * 2 * PI / 3);
            Vector2 wing = {-radial.y, radial.x};
            Vector2 base = Vector2Add(m->waypoint, Vector2Scale(radial, 70));
            Vector2 lamp = Vector2Subtract(base, Vector2Scale(radial, 12));
            DrawLineEx(Vector2Subtract(base, Vector2Scale(wing, 10)),
                       Vector2Add(base, Vector2Scale(wing, 10)), 7, (Color){90,107,118,255});
            DrawLineEx(base, lamp, 6, (Color){139,154,156,255});
            float glow = 0.7f + 0.3f * cosf(t * 3 - i * 2 * PI / 3);
            DrawCircleGradient(lamp, 13, Fade(beacon, 0.45f * glow), Fade(beacon, 0));
            DrawCircleV(lamp, 3, Fade(beacon, glow));
        }
        return;
    }
    DrawRing(m->waypoint, 43, 45, 0, 360, 40, Fade(beacon, 0.60f));
    for (int i = 0; i < 3; ++i) {
        float angle = i * 120 + t * 19;
        DrawRing(m->waypoint, 55, 58, angle, angle + 32, 12, beacon);
    }
    if (m->escortLeg == 0 && m->loadingTime > 0)
        DrawRing(m->waypoint, 48, 52, -90, -90 + 360 * m->loadingTime / cargoLoadDuration, 48, AMBER);
    Vector2 axis = Vector2Normalize(Vector2Subtract(m->waypoint, m->transport));
    if (Vector2LengthSqr(axis) < 0.01f)
        axis = Vector2Normalize(Vector2Subtract(m->escortRoute[1], m->escortRoute[0]));
    bool moving = !m->contested && Vector2Distance(game->player.position, m->transport) <= 290 &&
                  (m->escortLeg > 0 || m->loadingTime == 0);
    drawTransport(game, m->transport, axis, 78, moving ? 0.8f : 0.12f);
    if (m->escortLeg > 0 || m->loadingTime > 0) {
        Vector2 side = {-axis.y, axis.x};
        float loading = m->escortLeg ? 1 : m->loadingTime / cargoLoadDuration;
        Vector2 crate = Vector2Add(m->transport,Vector2Scale(side, 14 + 43 * (1-loading)));
        DrawRectanglePro((Rectangle){crate.x,crate.y,18,10},(Vector2){9,5},
                         atan2f(axis.y,axis.x)*RAD2DEG,(Color){182,155,111,255});
        DrawCircleV(crate,2,beacon);
    }
}
