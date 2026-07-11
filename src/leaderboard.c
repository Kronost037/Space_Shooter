#include "game.h"
#include "leaderboard.h"
#include "background.h"

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
#define LB_FILE_PATH ".game/leaderboard.bin"

#define INITIAL_CAPACITY 16
#define ROW_HEIGHT 45.0f
#define ROW_SPACING 5.0f
#define P_WIDTH 600.0f
#define P_HEIGHT 650.0f
#define LIST_HEIGHT 465.0f


static const Color COLOR_BG_GLASS      = { 10, 15, 30, 220 };
static const Color COLOR_PANEL_BORDER  = { 0, 150, 255, 180 };
static const Color COLOR_CYAN_GLOW     = { 0, 255, 255, 200 };
static const Color COLOR_PURPLE_GLOW   = { 180, 0, 255, 150 };
static const Color COLOR_TEXT_HEADER   = { 220, 240, 255, 255 };
static const Color COLOR_TEXT_MUTED    = { 120, 150, 180, 255 };
static const Color COLOR_ROW_HOVER     = { 20, 40, 80, 200 };
static const Color COLOR_SCROLLBAR     = { 0, 200, 255, 150 };

static const Color COLOR_GOLD          = { 255, 215, 0, 255 };
static const Color COLOR_SILVER        = { 192, 192, 192, 255 };
static const Color COLOR_BRONZE        = { 205, 127, 50, 255 };



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

static float clampFloat(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static float lerpFloat(float start, float end, float amount) {
    return start + amount * (end - start);
}

static float easeOutQuad(float t) {
    return t * (2.0f - t);
}

static void drawNeonGlow(Rectangle rec, Color color, int iterations) {
    for (int i = 0; i < iterations; i++) {
        Rectangle glowRec = {
            rec.x - i, rec.y - i,
            rec.width + i * 2, rec.height + i * 2
        };
        Color c = color;
        c.a = color.a / (i + 1);
        DrawRectangleRoundedLines(glowRec, 0.2f, 8, c);
    }
}

static void drawScrollbar(Rectangle panelRec, float scroll, float maxScroll, float contentHeight) {
    if (maxScroll <= 0) return;

    float viewRatio = panelRec.height / contentHeight;
    float trackHeight = panelRec.height - 20.0f;
    float thumbHeight = trackHeight * viewRatio;
    if (thumbHeight < 20.0f) thumbHeight = 20.0f;

    float scrollRatio = scroll / maxScroll;
    float thumbY = panelRec.y + 10.0f + scrollRatio * (trackHeight - thumbHeight);

    Rectangle trackRec = { panelRec.x + panelRec.width - 15.0f, panelRec.y + 10.0f, 4.0f, trackHeight };
    Rectangle thumbRec = { panelRec.x + panelRec.width - 16.0f, thumbY, 6.0f, thumbHeight };


    DrawRectangleRounded(trackRec, 1.0f, 4, (Color){ 255, 255, 255, 10 });
    

    DrawRectangleRounded(thumbRec, 1.0f, 8, COLOR_SCROLLBAR);
    drawNeonGlow(thumbRec, COLOR_CYAN_GLOW, 3);
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
    
    lb->scroll = 0.0f;
    lb->targetScroll = 0.0f;
    lb->animTimer = 0.0f;
    lb->isVisible = true;
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

void leaderboardLoad(Leaderboard *lb) {
    if (!lb) return;

    lb->count = 0;
    lb->scroll = 0.0f;
    lb->targetScroll = 0.0f;
    
    FILE *file = fopen(LB_FILE_PATH, "rb");
    if (!file) {
        return;
    }

    uint32_t magic = 0;
    uint32_t version = 0;

    if (fread(&magic, sizeof(uint32_t), 1, file) != 1 || magic != LB_MAGIC_HEADER) {
        fclose(file);
        return;
    }

    if (fread(&version, sizeof(uint32_t), 1, file) != 1 || version > LB_VERSION) {
        fclose(file);
        return;
    }

    int fileCount = 0;
    if (fread(&fileCount, sizeof(int), 1, file) != 1 || fileCount < 0) {
        fclose(file);
        return;
    }


    if (fileCount > lb->capacity) {
        lb->capacity = fileCount + INITIAL_CAPACITY;
        LeaderboardEntry *newEntries = (LeaderboardEntry *)realloc(lb->entries, lb->capacity * sizeof(LeaderboardEntry));
        if (!newEntries) {
            fclose(file);
            return;
        }
        lb->entries = newEntries;
    }

    if (fileCount > 0) {
        size_t readCount = fread(lb->entries, sizeof(LeaderboardEntry), fileCount, file);
        lb->count = (int)readCount;
    }

    fclose(file);
    
    qsort(lb->entries, lb->count, sizeof(LeaderboardEntry), compareEntries);

    lb->scroll = 0.0f;
    lb->targetScroll = 0.0f;
}

void leaderboardSave(const Leaderboard *lb) {
    if (!lb) return;

    ensureDirectory();

    FILE *file = fopen(LB_FILE_PATH, "wb");
    if (!file) return;
    
    uint32_t magic = LB_MAGIC_HEADER;
    uint32_t version = LB_VERSION;

    fwrite(&magic, sizeof(uint32_t), 1, file);
    fwrite(&version, sizeof(uint32_t), 1, file);
    fwrite(&lb->count, sizeof(int), 1, file);

    if (lb->count > 0) {
        fwrite(lb->entries, sizeof(LeaderboardEntry), lb->count, file);
    }

    fclose(file);
}

void leaderboardAddScore(Leaderboard *lb, const char *name, int score) {
    if (!lb) return;

    if (lb->count >= lb->capacity) {
        lb->capacity *= 2;
        LeaderboardEntry *newEntries = (LeaderboardEntry *)realloc(lb->entries, lb->capacity * sizeof(LeaderboardEntry));
        if (newEntries) {
            lb->entries = newEntries;
        } else {
            return;
        }
    }

    LeaderboardEntry *entry = &lb->entries[lb->count];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->score = score;
    entry->timestamp = time(NULL);

    lb->count++;
    
    qsort(lb->entries, lb->count, sizeof(LeaderboardEntry), compareEntries);
    leaderboardSave(lb);
}

void leaderboardUpdate(Leaderboard *lb) {
    if (!lb) return;

    float dt = GetFrameTime();


    if (lb->isVisible && lb->animTimer < 3.0f) {
        lb->animTimer += dt;
    }


    float contentHeight = lb->count * (ROW_HEIGHT + ROW_SPACING) - ROW_SPACING;
    float maxScroll = (contentHeight > LIST_HEIGHT) ? (contentHeight - LIST_HEIGHT) : 0.0f;
    
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f) {
        lb->targetScroll -= wheelMove * 60.0f;
    }

    lb->targetScroll = clampFloat(lb->targetScroll, 0.0f, maxScroll);
    lb->scroll = lerpFloat(lb->scroll, lb->targetScroll, dt * 12.0f);
}

void drawLeaderboard(Leaderboard *lb) {
    if (!lb || !lb->isVisible) return;
   
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();

    drawBackground(SCREEN_WIDTH, SCREEN_HEIGHT, lb->animTimer);
    

    float panelAnim = clampFloat(lb->animTimer / 0.5f, 0.0f, 1.0f);
    float easedPanelAnim = easeOutQuad(panelAnim);
    
    int globalAlpha = (int)(255 * easedPanelAnim);
    float yOffset = (1.0f - easedPanelAnim) * 50.0f;

    Rectangle panelRec = {
        (screenWidth - P_WIDTH) * 0.5f,
        (screenHeight - P_HEIGHT) * 0.5f + yOffset,
        P_WIDTH,
        P_HEIGHT
    };


    Color panelBg = COLOR_BG_GLASS;
    panelBg.a = (unsigned char)((panelBg.a * globalAlpha) / 255);
    DrawRectangleRounded(panelRec, 0.05f, 16, panelBg);


    Color borderCol = COLOR_PANEL_BORDER;
    borderCol.a = (unsigned char)((borderCol.a * globalAlpha) / 255);
    DrawRectangleRoundedLines(panelRec, 0.2f, 16, borderCol);
    
    Color glowCol = COLOR_CYAN_GLOW;
    glowCol.a = (unsigned char)((glowCol.a * globalAlpha) / 255);
    drawNeonGlow(panelRec, glowCol, 4);


    float currentY = panelRec.y + 30.0f;
    
    const char *title = "LEADERBOARD";
    int titleSize = 40;
    float titleWidth = (float)MeasureText(title, titleSize);
    
    Color titleColor = COLOR_TEXT_HEADER;
    titleColor.a = (unsigned char)globalAlpha;
    DrawText(title, (int)(panelRec.x + (panelRec.width - titleWidth) * 0.5f), (int)currentY, titleSize, titleColor);
    

    currentY += 50.0f;
    Color mutedColor = COLOR_TEXT_MUTED;
    mutedColor.a = (unsigned char)globalAlpha;
    DrawText("TOP OPERATIVES", (int)(panelRec.x + panelRec.width * 0.5f - MeasureText("TOP OPERATIVES", 20) * 0.5f), (int)currentY, 20, mutedColor);
    
    currentY += 30.0f;
    Color lineCol = COLOR_PURPLE_GLOW;
    lineCol.a = (unsigned char)globalAlpha;
    DrawLineEx((Vector2){panelRec.x + 40, currentY}, (Vector2){panelRec.x + panelRec.width - 40, currentY}, 2.0f, lineCol);


    currentY += 15.0f;
    DrawText("RANK", (int)(panelRec.x + 60), (int)currentY, 20, mutedColor);
    DrawText("OPERATIVE", (int)(panelRec.x + 180), (int)currentY, 20, mutedColor);
    DrawText("SCORE", (int)(panelRec.x + panelRec.width - 150), (int)currentY, 20, mutedColor);

    currentY += 30.0f;


    float listHeight = LIST_HEIGHT;
    Rectangle listRec = { panelRec.x + 20, currentY, panelRec.width - 40, listHeight };
    
    BeginScissorMode((int)listRec.x, (int)listRec.y, (int)listRec.width, (int)listRec.height);

    Vector2 mousePos = GetMousePosition();

    for (int i = 0; i < lb->count; i++) {

        float rowDelay = 0.3f + (i * 0.05f);
        float rowAnim = clampFloat((lb->animTimer - rowDelay) / 0.4f, 0.0f, 1.0f);
        float easedRowAnim = easeOutQuad(rowAnim);
        
        if (easedRowAnim <= 0.0f) continue;

        float rowY = currentY + (i * (ROW_HEIGHT + ROW_SPACING)) - lb->scroll;
        

        if (rowY + ROW_HEIGHT < listRec.y) continue;
        if (rowY > listRec.y + listRec.height) break;

        Rectangle rowRec = { panelRec.x + 40, rowY, panelRec.width - 80, ROW_HEIGHT };


        bool isHovered = CheckCollisionPointRec(mousePos, rowRec) && CheckCollisionPointRec(mousePos, listRec);
        
        float rowAlpha = globalAlpha * easedRowAnim / 255.0f;


        if (isHovered) {
            Color hoverCol = COLOR_ROW_HOVER;
            hoverCol.a = (unsigned char)(hoverCol.a * rowAlpha);
            DrawRectangleRounded(rowRec, 0.2f, 8, hoverCol);
            
            Color hoverBorder = COLOR_CYAN_GLOW;
            hoverBorder.a = (unsigned char)(100 * rowAlpha);
            DrawRectangleRoundedLines(rowRec, 0.2f, 8, hoverBorder);
        }


        Color rankColor = COLOR_CYAN_GLOW;
        if (i == 0) rankColor = COLOR_GOLD;
        else if (i == 1) rankColor = COLOR_SILVER;
        else if (i == 2) rankColor = COLOR_BRONZE;
        
        rankColor.a = (unsigned char)(rankColor.a * rowAlpha);

        Color rowTextColor = COLOR_TEXT_HEADER;
        rowTextColor.a = (unsigned char)(rowTextColor.a * rowAlpha);


        char rankStr[16];
        snprintf(rankStr, sizeof(rankStr), "#%d", i + 1);
        DrawText(rankStr, (int)(rowRec.x + 20), (int)(rowRec.y + 12), 20, rankColor);


        DrawText(lb->entries[i].name, (int)(rowRec.x + 140), (int)(rowRec.y + 12), 20, rowTextColor);


        char scoreStr[32];
        snprintf(scoreStr, sizeof(scoreStr), "%08d", lb->entries[i].score);
        int scoreWidth = MeasureText(scoreStr, 20);
        DrawText(scoreStr, (int)(rowRec.x + rowRec.width - scoreWidth - 20), (int)(rowRec.y + 12), 20, rowTextColor);
    }

    EndScissorMode();

    if (lb->count == 0) {
        DrawText(
                 "   Such Emptiness...",
                 (int)(panelRec.x + panelRec.width / 2 - 230.0f), (int)currentY, 40,
                 COLOR_TEXT_MUTED
                 );
    }
    
    float contentHeight = lb->count * (ROW_HEIGHT + ROW_SPACING) - ROW_SPACING;
    float maxScroll = (contentHeight > listRec.height) ? (contentHeight - listRec.height) : 0.0f;
    drawScrollbar(listRec, lb->scroll, maxScroll, contentHeight);
}
