#ifndef MENU_H_
#define MENU_H_

#include "core.h"
#include <stdbool.h>

typedef struct S_button {
    Rectangle bounds;
    const char *label;
} Button;

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_START,
    MENU_ACTION_LEADERBOARD,
    MENU_ACTION_SETTING,
    MENU_ACTION_EXIT
} MenuAction;

typedef enum {
    MENU_PHASE_IDLE = 0,
    MENU_PHASE_TRANSITION
} MenuPhase;

struct S_menu {
    Music bg_song;
    Sound hover_sfx;
    Sound click_sfx;

    Vector2 mousePos;

    Button startButton;
    Button leaderboardButton;
    Button settingButton;
    Button exitButton;

    float timer;
    float titlePulse;

    float transitionTimer;
    float transitionDuration;
    MenuPhase phase;
    MenuAction pendingAction;

    int hoveredButton;
    float hoverPulse;
    float clickFlash;
};

void updateMenuLayout(Game *game);
void refreshLayout(Game *game);
void initializeMenu(Menu *menu);
void runMenuPhysics(Game *game);
void drawMenu(Game *game);

bool menuActionReady(const Menu *menu);
MenuAction menuConsumeAction(Menu *menu);

#endif
