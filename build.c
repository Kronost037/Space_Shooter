#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMD_SIZE 4096

#define raylib_url "https://github.com/raysan5/raylib/archive/refs/tags/6.0.zip"

static int command_exists(const char *cmd)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "command -v %s >/dev/null 2>&1", cmd);

    return system(buffer) == 0;
}

static void fetch_raylib(void)
{

    if(access("raylib/include/raylib.h", F_OK)  == 0  &&
       access("raylib/include/raymath.h", F_OK) == 0  &&
       access("raylib/lib/libraylib.a", F_OK)   == 0) {
        puts("INFO: Raylib is already installed.");
        return;
    }
    
    int exit = 0;
    
    if (!command_exists("wget")) {
        puts("Error: wget not found.");
        exit = 1;
    }

    if (!command_exists("cmake")) {
        puts("Error: cmake not found.");
        exit = 1;
    }

    if (!command_exists("unzip")) {
        puts("Error: unzip not found.");
        exit = 1;
    }

    if(exit) {
        puts("INFO: Some building tools were not found. Install them before building.");
        puts("INFO: Input the names of missing tools in the following command.");
        puts("INFO: sudo apt install <name>");
        puts("INFO: Ex. sudo apt install wget cmake unzip");
        return;
    }
    
    puts("INFO: [*] Downloading Raylib...");

    char *cmd = "wget -O raylib-6.0.zip "raylib_url; 

    if (system(cmd))
    {
        puts("INFO: Failed to download Raylib.");
        return;
    }

    puts("INFO: Building Raylib locally...");

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
        puts("INFO: Failed to build Raylib.");
        puts("INFO: Might be missing X11 dev-tools. Try the following command,");
        puts("INFO: sudo apt install xorg-dev");
        return;
    }

    puts("INFO: Raylib files installed locally in the following locations:");
    puts("     ./raylib/include");
    puts("     ./raylib/lib");
}

typedef enum {
    DEBUG_OFF = 0,
    DEBUG_ON
} Debug_mode;

static int build_game(Debug_mode debug)
{
    char cmd[CMD_SIZE];

    snprintf(cmd, sizeof(cmd),
            "gcc "
            "-std=c17 "
            "%s "
            "-Wall -Wextra "
            "%s "
            "src/background.c src/menu.c src/leaderboard.c src/game.c "
            "-Iraylib/include "
            "-Lraylib/lib "
            "-lraylib "
            "-lm -lX11 "
            "-o \"Space Shooter\"",
            debug ? "-O0 -g3" : "-O2",
            debug ? "-Wpedantic -Wconversion -Wshadow "
                    "-Wformat=2 -fsanitize=address,undefined"
                    : "");

    printf("Running:\n%s\n\n", cmd);

    if(system(cmd) != 0) {
        puts("");
        puts("INFO: If you are having problem downloading raylib. The following command does it for you.");
        puts("INFO: ./build --download raylib");
        return -1;
    }

    return 0;
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
        return build_game(DEBUG_OFF);
    }

    if (strcmp(argv[1], "--release") == 0) {
        return build_game(DEBUG_OFF);
    }

    if (strcmp(argv[1], "--debug") == 0) {
        return build_game(DEBUG_ON);
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

        printf("INFO: Unknown dependency: %s\n", argv[2]);
        puts("INFO: Only supports raylib");
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    puts("INFO: Incorrect usage. Try following command for help");
    puts("INFO: ./build --help");
    return 1;
}
