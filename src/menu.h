#ifndef MENU_H
#define MENU_H

#include "core.h"

typedef enum {
    MENU_ACTION_NONE, MENU_ACTION_START, MENU_ACTION_LEADERBOARD, MENU_ACTION_SETTING, MENU_ACTION_EXIT
} MenuAction;

struct S_menu {
    Music bg_song;
    Sound hover_sfx, click_sfx;
    int focus;
    float time, transition;
    MenuAction pending;
};

void initializeMenu(Menu *menu);
void runMenuPhysics(Game *game);
void drawMenu(Game *game);
bool menuActionReady(const Menu *menu);
MenuAction menuConsumeAction(Menu *menu);

#endif
