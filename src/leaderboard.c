#define _POSIX_C_SOURCE 200809L
#include "save_file.h"
#include <errno.h>
#include "game.h"
#include "leaderboard.h"
#include "ui/widgets.h"
#include "ui/display.h"

#if defined(_WIN32)
    #include <direct.h>
    #define MAKE_DIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MAKE_DIR(path) mkdir(path, 0777)
#endif


#define LB_MAGIC_HEADER 0x4C445242
#define LB_VERSION 1
#define LB_FILE_DIR ".game"
#define LB_FILE_PATH ".game/sector-siege-leaderboard.bin"

#define INITIAL_CAPACITY 16
#define MAX_ENTRIES 500
#define PAGE_SIZE 8

static void ensureDirectory(void) {
    MAKE_DIR(LB_FILE_DIR);
}

static int compareEntries(const void *a, const void *b) {
    const LeaderboardEntry *ea = (const LeaderboardEntry *)a;
    const LeaderboardEntry *eb = (const LeaderboardEntry *)b;

    if (ea->score > eb->score) return -1;
    if (ea->score < eb->score) return 1;

    if (ea->timestamp < eb->timestamp) return -1;
    if (ea->timestamp > eb->timestamp) return 1;

    return 0;
}

void leaderboardInit(Leaderboard *lb) {
    if (!lb) return;
    
    lb->count = 0;
    lb->capacity = INITIAL_CAPACITY;
    lb->entries = (LeaderboardEntry *)malloc(lb->capacity * sizeof(LeaderboardEntry));
    if (!lb->entries) {
        lb->capacity = 0;
        return;
    }
    
    lb->page = 0;

}

void leaderboardShutdown(Leaderboard *lb) {
    if (!lb) return;
    
    if (lb->entries) {
        free(lb->entries);
        lb->entries = NULL;
    }
    lb->count = 0;
    lb->capacity = 0;
}

static bool reserveEntries(Leaderboard *lb, int capacity) {
    if (capacity <= lb->capacity) return true;
    LeaderboardEntry *entries = realloc(lb->entries, (size_t)capacity * sizeof *entries);
    if (!entries) return false;
    lb->entries = entries;
    lb->capacity = capacity;
    return true;
}

void leaderboardLoad(Leaderboard *lb) {
    if (!lb) return;

    lb->count = 0;
    lb->page = 0;
    
    FILE *file = fopen(LB_FILE_PATH, "rb");
    if (!file && errno == ENOENT) file = fopen(".game/leaderboard.bin", "rb");
    if (!file) {
        return;
    }

    uint32_t magic = 0;
    uint32_t version = 0;

    if (fread(&magic, sizeof(uint32_t), 1, file) != 1 || magic != LB_MAGIC_HEADER) {
        fclose(file);
        return;
    }

    if (fread(&version, sizeof(uint32_t), 1, file) != 1 || version != LB_VERSION) {
        fclose(file);
        return;
    }

    int fileCount = 0;
    if (fread(&fileCount, sizeof(int), 1, file) != 1 || fileCount < 0 || fileCount > MAX_ENTRIES) {
        fclose(file);
        return;
    }


    if (!reserveEntries(lb, fileCount)) {
        fclose(file);
        return;
    }
    if (fileCount > 0) {
        size_t count = (size_t)fileCount;
        lb->count = fread(lb->entries, sizeof *lb->entries, count, file) == count ? fileCount : 0;
        for (int i = 0; i < lb->count; ++i) lb->entries[i].name[sizeof lb->entries[i].name - 1] = '\0';
    }

    fclose(file);
    
    if (lb->count > 1) qsort(lb->entries, (size_t)lb->count, sizeof *lb->entries, compareEntries);

    lb->page = 0;
}

bool leaderboardSave(const Leaderboard *lb) {
    ensureDirectory();
    char temporary[512];
    FILE *file = openSaveTemporary(temporary, sizeof temporary);
    if (!file) return false;
    uint32_t header[] = {LB_MAGIC_HEADER, LB_VERSION};
    bool ok = fwrite(header,sizeof header,1,file) == 1 &&
              fwrite(&lb->count,sizeof lb->count,1,file) == 1 &&
              fwrite(lb->entries,sizeof *lb->entries,(size_t)lb->count,file) == (size_t)lb->count;
    if (fclose(file) != 0) ok = false;
    if (ok && replaceSaveFile(temporary, LB_FILE_PATH)) return true;
    remove(temporary);
    return false;
}

bool leaderboardAddScore(Leaderboard *lb, const char *name, int score) {
    if (!lb) return false;
    LeaderboardEntry entry = { .score = score, .timestamp = time(NULL) };
    snprintf(entry.name, sizeof entry.name, "%s", name);
    if (lb->count == MAX_ENTRIES) {
        if (compareEntries(&entry, &lb->entries[MAX_ENTRIES - 1]) >= 0) return true;
        lb->entries[MAX_ENTRIES - 1] = entry;
    } else {
        if (lb->count == lb->capacity) {
            int capacity = lb->capacity > 0 ? lb->capacity * 2 : INITIAL_CAPACITY;
            if (!reserveEntries(lb, capacity < MAX_ENTRIES ? capacity : MAX_ENTRIES)) return false;
        }
        lb->entries[lb->count++] = entry;
    }

    qsort(lb->entries, (size_t)lb->count, sizeof *lb->entries, compareEntries);
    if (leaderboardSave(lb)) return true;
    leaderboardLoad(lb);
    return false;
}

static const Rectangle previous = {596, 726, 178, 54};
static const Rectangle next = {784, 726, 142, 54};
static const Rectangle back = {102, 726, 225, 54};

void leaderboardUpdate(Leaderboard *lb) {
    float wheel = GetMouseWheelMove();
    int dir = (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_DOWN) || wheel < 0 || uiClicked(next)) -
              (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP) || wheel > 0 || uiClicked(previous));
    int last = lb->count ? (lb->count - 1) / PAGE_SIZE : 0;
    lb->page = (int)Clamp(lb->page + dir, 0, last);
}

bool leaderboardBack(void) {
    return IsKeyPressed(KEY_ESCAPE) || uiClicked(back);
}

void drawLeaderboard(const Game *game) {
    const Leaderboard *lb = &game->leaderboard;
    uiBackdrop(game, (float)GetTime());
    DrawRectangle(0, 0, 930, SCREEN_HEIGHT, (Color){6,13,21,201});
    DrawRectangleGradientH(930, 0, SCREEN_WIDTH - 930, SCREEN_HEIGHT, (Color){6,13,21,201},
                           (Color){6,13,21,0});
    uiTitle(game, "Flight records", 100, 98, 78);
    if (lb->count) {
        uiLabel(game, "Rank", 112, 246, 24, UI_MUTED);
        uiLabel(game, "Callsign", 254, 246, 24, UI_MUTED);
        uiLabel(game, "Score", 925 - uiTextWidth(game, "Score", 24), 246, 24, UI_MUTED);
    }
    Rectangle viewport = screenViewport();
    float scale = viewport.width / SCREEN_WIDTH;
    // Snap both boundaries in window space; a logical pixel can vanish after scaling.
    float thickness = fmaxf(1, roundf(1.5f * scale)) / scale;
    for (int row = 0; row < PAGE_SIZE; ++row) {
        int i = lb->page * PAGE_SIZE + row;
        if (i >= lb->count) break;
        float y = 303 + 49 * row;
        float edge = (roundf(viewport.y + (y + 39) * scale) - viewport.y) / scale;
        DrawRectangleRec((Rectangle){112, edge, 813, thickness}, (Color){98,120,135,255});
        uiLabel(game, TextFormat("%02d", i + 1), 112, y, 24, i == 0 ? UI_AMBER : UI_MUTED);
        uiLabel(game, lb->entries[i].name, 254, y, 24, UI_PAPER);
        const char *score = TextFormat("%d", lb->entries[i].score);
        uiLabel(game, score, 925 - uiTextWidth(game, score, 24), y, 24, i == 0 ? UI_AMBER : UI_PAPER);
    }
    if (!lb->count) uiLabel(game, "No flights recorded", 112, 323, 28, UI_PAPER);
    uiAction(game, back, "Main menu", CheckCollisionPointRec(GetMousePosition(), back));
    if (lb->page) uiAction(game, previous, "Previous", CheckCollisionPointRec(GetMousePosition(), previous));
    if ((lb->page + 1) * PAGE_SIZE < lb->count) uiAction(game, next, "Next", CheckCollisionPointRec(GetMousePosition(), next));
}
