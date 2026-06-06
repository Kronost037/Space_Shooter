#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMD_SIZE 4096

#define raylib_url "https://github.com/raysan5/raylib/archive/refs/tags/6.0.zip"

static int exists(const char *path)
{
    return access(path, F_OK) == 0;
}

static int command_exists(const char *cmd)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "command -v %s >/dev/null 2>&1", cmd);

    return system(buffer) == 0;
}

static void fetch_raylib(void)
{
    if (!command_exists("wget")) {
        puts("Error: wget not found.");
        return;
    }

    if (!command_exists("cmake")) {
        puts("Error: cmake not found.");
        return;
    }

    if (!command_exists("unzip")) {
        puts("Error: unzip not found.");
        return;
    }

    puts("[*] Downloading Raylib...");

    char *cmd = "wget -O raylib-6.0.zip "raylib_url; 

    if (system(cmd))
    {
        puts("Failed to download Raylib.");
        return;
    }

    puts("[*] Building Raylib locally...");

    const char *build_cmd =
        "unzip raylib-6.0.zip && "
        "cd raylib-6.0 && "
        "mkdir build && "
        "cd build && "
        "cmake "
        "-DBUILD_SHARED_LIBS=OFF "
        "-DCMAKE_BUILD_TYPE=Release "
        "-DCMAKE_INSTALL_PREFIX=../../raylib "
        ".. && "
        "cmake --build . -j$(nproc) && "
        "cmake --install . && "
        "cd ../.. && "
        "rm -rf raylib-6.0 raylib-6.0.zip";

    if (system(build_cmd)) {
        puts("Failed to build Raylib.");
        return;
    }

    puts("[+] Raylib installed locally:");
    puts("    ./raylib/include");
    puts("    ./raylib/lib");
}

static int build_game(int debug)
{
    char cmd[CMD_SIZE];

    const int has_local_raylib =
        exists("raylib/include/raylib.h") &&
        exists("raylib/lib/libraylib.a");

    if (has_local_raylib) {
        snprintf(cmd, sizeof(cmd),
            "gcc "
            "-std=c17 "
            "%s "
            "-Wall -Wextra "
            "%s "
            "game.c "
            "-Iraylib/include "
            "-Lraylib/lib "
            "-lraylib "
            "-lm -lX11 "
            "-o \"Space Shooter\"",
            debug ? "-O0 -g3" : "-O2",
            debug ? "-Wpedantic -Wconversion -Wshadow "
                    "-Wformat=2 -fsanitize=address,undefined"
                    : "");
    } else {
        snprintf(cmd, sizeof(cmd),
            "gcc "
            "-std=c17 "
            "%s "
            "-Wall -Wextra "
            "%s "
            "game.c "
            "-lraylib "
            "-lm "
            "-o \"Space Shooter\"",
            debug ? "-O0 -g3" : "-O2",
            debug
                ? "-Wpedantic -Wconversion -Wshadow "
                  "-Wformat=2 -fsanitize=address,undefined"
                : "");
    }

    printf("Running:\n%s\n\n", cmd);

    return system(cmd);
}

static void print_help(void)
{
    puts("HELP MENU");
    puts("");
    puts("./build                    Release build");
    puts("./build --debug            Debug build");
    puts("./build --download raylib  Download Raylib locally");
    puts("./build --help             Show this help");
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        return build_game(0);
    }

    if (strcmp(argv[1], "--release") == 0) {
        return build_game(0);
    }

    if (strcmp(argv[1], "--debug") == 0) {
        return build_game(1);
    }

    if (strcmp(argv[1], "--download") == 0) {
        if (argc < 3) {
            puts("Usage: ./build --download raylib");
            return 1;
        }

        if (strcmp(argv[2], "raylib") == 0) {
            fetch_raylib();
            return 0;
        }

        printf("Unknown dependency: %s\n", argv[2]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    puts("Incorrect usage.");
    puts("Try: ./build --help");
    return 1;
}
