#include "mission.h"
#include "flight.h"

static const Color beacon = {134,223,207,255};

void startMission(Game *game, Vector2 launch) {
    game->mission = (Mission){.stage = MISSION_SURVEY,
                              .waypoint = Vector2Add(launch, (Vector2){-190,-100})};
}

void beginEscort(Game *game, Vector2 relay) {
    Mission *m = &game->mission;
    m->stage = MISSION_ESCORT;
    m->transport = relay;
    Vector2 route = Vector2Normalize(Vector2Subtract(game->elites[MAX_ELITES - 1].ship.position, relay));
    m->waypoint = Vector2Add(relay, Vector2Scale(route, 760));
    game->dispatch = "Mercy Six: engines live. Stay close and clear our route.";
    game->dispatchTime = 4;
}

void advanceMission(Game *game, float dt) {
    Mission *m = &game->mission;
    if (m->stage == MISSION_SURVEY && Vector2Distance(game->player.position, m->waypoint) < 72) {
        burst(game, m->waypoint, beacon, 14);
        game->payoutPosition = m->waypoint;
        award(game, 15);
        if (++m->fixes == 1) {
            m->waypoint = Vector2Add(m->waypoint, (Vector2){-320,-215});
        } else {
            m->stage = MISSION_RELAYS;
            for (int i = 0; i < MAX_ELITES - 1; ++i) game->elites[i].unlocked = true;
            game->dispatch = "Aster Lead: signal fixed. Three jammers on your radar.";
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
        burst(game, m->transport, beacon, 32);
        game->payoutPosition = m->transport;
        award(game, 90);
        game->elites[MAX_ELITES - 1].unlocked = true;
        m->stage = MISSION_CARRIER;
        game->dispatch = "Mercy Six is clear. Carrier transmitting on the jump corridor.";
        game->dispatchTime = 3.8f;
    }
}

const char *missionTitle(const Game *game) {
    switch (game->mission.stage) {
    case MISSION_SURVEY: return TextFormat("I   /   DEAD AIR     -     Reach beacon %d of 2", game->mission.fixes + 1);
    case MISSION_RELAYS: return TextFormat("II   /   BREAKWATER     -     Wardens %d of 3", game->wardensCleared);
    case MISSION_ESCORT: return game->mission.contested ? "III   /   MERCY RUN     -     Clear nearby scouts" :
        "III   /   MERCY RUN     -     Escort the hospital ship";
    case MISSION_CARRIER: return "IV   /   LAST CROSSING     -     Destroy the Carrier";
    }
    return "";
}

bool missionWaypoint(const Game *game, Vector2 *position) {
    if (game->mission.stage == MISSION_SURVEY) *position = game->mission.waypoint;
    else if (game->mission.stage == MISSION_ESCORT) *position = game->mission.transport;
    else return false;
    return true;
}

void drawMission(const Game *game) {
    const Mission *m = &game->mission;
    if (m->stage != MISSION_SURVEY && m->stage != MISSION_ESCORT) return;
    float t = (float)game->elapsedTime;
    DrawRing(m->waypoint, 43, 45, 0, 360, 40, Fade(beacon, 0.60f));
    for (int i = 0; i < 3; ++i) {
        float angle = i * 120 + t * 19;
        DrawRing(m->waypoint, 55, 58, angle, angle + 32, 12, beacon);
    }
    if (m->stage != MISSION_ESCORT) return;
    Vector2 p = m->transport;
    Vector2 axis = Vector2Normalize(Vector2Subtract(m->waypoint, p));
    Vector2 side = {-axis.y,axis.x};
    for (int i = -1; i <= 1; i += 2) {
        Vector2 pod = Vector2Add(p, Vector2Scale(side, 14 * i));
        DrawLineEx(Vector2Subtract(pod, Vector2Scale(axis, 19)), Vector2Add(pod, Vector2Scale(axis, 5)), 9, (Color){128,152,166,255});
    }
    DrawLineEx(Vector2Subtract(p, Vector2Scale(axis, 25)), Vector2Add(p, Vector2Scale(axis, 26)), 18, RAYWHITE);
    DrawLineEx(Vector2Subtract(p, Vector2Scale(side, 7)), Vector2Add(p, Vector2Scale(side, 7)), 4, DARKGREEN);
    DrawLineEx(Vector2Subtract(p, Vector2Scale(axis, 7)), Vector2Add(p, Vector2Scale(axis, 7)), 4, DARKGREEN);
}
