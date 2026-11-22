#include "rules.h"

const FlightRules flightRules[DIFFICULTY_COUNT] = {
    [EASY]   = {"Easy",   "Slower patrols and longer attack warnings.", 20, 36, 0.82f, 1.25f, 0.84f, 1.24f, 80},
    [MEDIUM] = {"Medium", "Standard patrols and attack warnings.",      26, 46, 1.00f, 1.00f, 1.00f, 1.00f, 100},
    [HARD]   = {"Hard",   "Faster salvos and heavier patrols.",          32, 56, 1.10f, 0.85f, 1.12f, 0.90f, 125},
    [INSANE] = {"Insane", "Dense patrols and short attack warnings.",    38, 66, 1.18f, 0.72f, 1.22f, 0.80f, 150},
};
