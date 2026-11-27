#define _POSIX_C_SOURCE 200809L
#include "save_file.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#endif

bool replaceSaveFile(const char *temporary, const char *destination) {
#ifdef _WIN32
    return MoveFileExA(temporary, destination, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temporary, destination) == 0;
#endif
}

FILE *openSaveTemporary(char *path, size_t capacity) {
#ifdef _WIN32
    if (capacity < MAX_PATH || !GetTempFileNameA(".game", "scr", 0, path)) return NULL;
    FILE *file = fopen(path, "wb");
#else
    if (snprintf(path, capacity, ".game/save-XXXXXX") >= (int)capacity) return NULL;
    int fd = mkstemp(path);
    if (fd < 0) return NULL;
    FILE *file = fdopen(fd, "wb");
    if (!file) close(fd);
#endif
    if (!file) remove(path);
    return file;
}
