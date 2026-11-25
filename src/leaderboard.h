#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include "core.h"
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

typedef struct {
    char name[32];
    int score;
    time_t timestamp;
} LeaderboardEntry;

typedef struct {
    LeaderboardEntry *entries;
    int count;
    int capacity;

    int page;
} Leaderboard;

void leaderboardInit(Leaderboard *lb);
void leaderboardShutdown(Leaderboard *lb);

void leaderboardLoad(Leaderboard *lb);
bool leaderboardSave(const Leaderboard *lb);

bool leaderboardAddScore(Leaderboard *lb, const char *name, int score);

void leaderboardUpdate(Leaderboard *lb);
bool leaderboardBack(void);
void drawLeaderboard(const Game *game);

#endif
