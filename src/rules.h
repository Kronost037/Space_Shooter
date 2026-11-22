#ifndef RULES_H
#define RULES_H

#include <stdbool.h>

typedef enum { EASY, MEDIUM, HARD, INSANE, DIFFICULTY_COUNT } Difficulty;

typedef struct {
    const char *name;
    const char *briefing;
    int wardenHull, carrierHull;
    float scoutSpeed, spawnInterval, boltSpeed, warningTime;
    int scorePercent;
} FlightRules;

extern const FlightRules flightRules[DIFFICULTY_COUNT];

typedef struct {
    Difficulty difficulty;
    int timerMinutes;
    bool timerEnabled;
    int volume, music, effects;
} Settings;

#define DEFAULT_SETTINGS ((Settings){ MEDIUM, 5, false, 80, 65, 85 })

#endif
