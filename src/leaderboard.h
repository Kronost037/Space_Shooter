#ifndef LEADERBOARD_H
#define LEADERBOARD_H

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

    float scroll;
    float targetScroll;    

    float animTimer;
    float globalTimer;
    bool isVisible;
} Leaderboard;

void leaderboardInit(Leaderboard *lb);
void leaderboardShutdown(Leaderboard *lb);

void leaderboardLoad(Leaderboard *lb);
void leaderboardSave(const Leaderboard *lb);

void leaderboardAddScore(Leaderboard *lb, const char *name, int score);

void leaderboardUpdate(Leaderboard *lb);
void drawLeaderboard(Leaderboard *lb);

#endif
