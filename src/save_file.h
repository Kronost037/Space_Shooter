#ifndef SAVE_FILE_H
#define SAVE_FILE_H
#include <stdbool.h>
#include <stdio.h>

bool replaceSaveFile(const char *temporary, const char *destination);
FILE *openSaveTemporary(char *path, size_t capacity);
#endif
