#include "save_file.h"
#include "settings.h"
#include "menu.h"
#include "ui/widgets.h"

#define SETTINGS_PATH ".game/flight-settings.txt"

enum { ROW_DIFFICULTY, ROW_TIMER, ROW_DURATION, ROW_VOLUME, ROW_MUSIC, ROW_EFFECTS, ROW_DONE };

static void applyAudio(const Game *game) {
    SetMasterVolume(game->settings.volume / 100.0f);
    SetMusicVolume(game->menu.bg_song, game->settings.music / 100.0f);
    float gain = game->settings.effects / 100.0f;
    SetSoundVolume(game->menu.hover_sfx, gain * 0.55f);
    SetSoundVolume(game->menu.click_sfx, gain * 0.65f);
    SetSoundVolume(game->laserSound, gain * 0.32f);
    SetSoundVolume(game->warningSound, gain * 0.44f);
    SetSoundVolume(game->damageSound, gain * 0.47f);
    SetSoundVolume(game->clearSound, gain * 0.30f);
    SetSoundVolume(game->pulseSound, gain * 0.40f);
}

void loadSettings(Game *game) {
    FILE *file = fopen(SETTINGS_PATH, "r");
    if (file) {
        int difficulty, enabled, minutes, master, music, fx;
        if (fscanf(file, "v1 %4d %4d %4d %4d %4d %4d", &difficulty, &enabled,
                   &minutes, &master, &music, &fx) == 6 &&
            difficulty >= EASY && difficulty < DIFFICULTY_COUNT && (enabled == 0 || enabled == 1) &&
            minutes >= 1 && minutes <= 60 && master >= 0 && master <= 100 &&
            music >= 0 && music <= 100 && fx >= 0 && fx <= 100) {
            game->settings = (Settings){(Difficulty)difficulty, minutes, enabled != 0, master, music, fx};
        }
        fclose(file);
    }
    applyAudio(game);
}

static void saveSettings(const Settings *s) {
    if (!DirectoryExists(".game")) MakeDirectory(".game");
    char temporary[512];
    FILE *f = openSaveTemporary(temporary, sizeof temporary);
    if (!f) return;
    int written = fprintf(f, "v1 %d %d %d %d %d %d\n", (int)s->difficulty,
                          (int)s->timerEnabled, s->timerMinutes, s->volume, s->music, s->effects);
    int closed = fclose(f);
    if (written > 0 && closed == 0 && replaceSaveFile(temporary, SETTINGS_PATH)) return;
    remove(temporary);
}

static Rectangle control(int row, bool plus) {
    return (Rectangle){plus ? 1097 : 750, 157.0f + row * 87.0f, 60, 52};
}

static const Rectangle done = {898, 714, 259, 56};

static void adjust(Game *game, int row, int dir) {
    Settings *s = &game->settings;
    switch (row) {
    case ROW_DIFFICULTY: s->difficulty = (Difficulty)(int)Clamp((int)s->difficulty + dir, EASY, INSANE); break;
    case ROW_TIMER: s->timerEnabled = !s->timerEnabled; break;
    case ROW_DURATION: s->timerMinutes = (int)Clamp(s->timerMinutes + dir, 1, 60); break;
    case ROW_VOLUME: s->volume = (int)Clamp(s->volume + dir * 5, 0, 100); break;
    case ROW_MUSIC: s->music = (int)Clamp(s->music + dir * 5, 0, 100); break;
    case ROW_EFFECTS: s->effects = (int)Clamp(s->effects + dir * 5, 0, 100); break;
    }
    applyAudio(game);
    PlaySound(game->menu.click_sfx);
}

void updateSettings(Game *game) {
    int *row = &game->settingRow;
    int vertical = IsKeyPressed(KEY_DOWN) - IsKeyPressed(KEY_UP);
    if (vertical) {
        do { *row = (*row + vertical + ROW_DONE + 1) % (ROW_DONE + 1); }
        while (*row == ROW_DURATION && !game->settings.timerEnabled);
    }
    if (IsKeyPressed(KEY_ESCAPE) || uiClicked(done) ||
        (*row == ROW_DONE && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)))) {
        saveSettings(&game->settings);
        PlaySound(game->menu.click_sfx);
        game->currentState = STATE_MENU;
        return;
    }
    for (int i = 0; i < ROW_DONE; ++i) {
        if (i == ROW_DURATION && !game->settings.timerEnabled) continue;
        int change = uiClicked(control(i, true)) - uiClicked(control(i, false));
        if (change) {
            *row = i;
            adjust(game, i, change);
            return;
        }
    }
    if (*row == ROW_DONE) return;
    int dir = (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) -
              (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT));
    if (*row == ROW_TIMER && IsKeyPressed(KEY_ENTER)) dir = 1;
    if (dir) adjust(game, *row, dir);
}

void drawSettings(const Game *game) {
    const Settings *s = &game->settings;
    static const char *labels[] = {"Difficulty", "Flight timer", "Time limit", "Master volume", "Menu music", "Sound effects"};
    const FlightRules *rules = &flightRules[s->difficulty];
    uiBackdrop(game, (float)GetTime());
    uiSurface((Rectangle){264, 57, 972, 751});
    uiTitle(game, "Settings", 325, 69, 60);
    for (int i = 0; i < ROW_DONE; ++i) {
        Rectangle left = control(i, false), right = control(i, true);
        bool enabled = i != ROW_DURATION || s->timerEnabled;
        bool selected = enabled && game->settingRow == i;
        Color ink = enabled ? UI_PAPER : UI_MUTED;
        if (selected) DrawRectangle(306, (int)left.y, 4, 52, UI_AMBER);
        uiLabel(game, labels[i], 325, left.y + 6, 29, ink);
        const char *value;
        switch (i) {
        case ROW_DIFFICULTY: value = rules->name; break;
        case ROW_TIMER: value = s->timerEnabled ? "On" : "Off"; break;
        case ROW_DURATION: value = TextFormat("%d min", s->timerMinutes); break;
        default: value = TextFormat("%d%%", i == ROW_VOLUME ? s->volume : i == ROW_MUSIC ? s->music : s->effects); break;
        }
        float width = uiTextWidth(game, value, 27);
        uiLabel(game, value, 953 - width / 2, left.y + 10, 27, ink);
        if (enabled) {
            uiButton(game, left, "-", CheckCollisionPointRec(GetMousePosition(), left));
            uiButton(game, right, "+", CheckCollisionPointRec(GetMousePosition(), right));
        }
        DrawRectangle(325, (int)left.y + 67, 832, 1, (Color){52,68,81,255});
    }
    uiLabel(game, TextFormat("%d%% scoring", rules->scorePercent), 325, 733, 20, UI_ICE);
    uiButton(game, done, "Done", game->settingRow == ROW_DONE || CheckCollisionPointRec(GetMousePosition(), done));
}
