#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *release_build = 
		"gcc -std=c17 -O2 "
		"-Wall -Wextra "
		"game.c -o \"Space Shooter\" "
		"-lraylib -lm";
	
const char *debug_build = 
		"gcc -std=c17 -O0 -g3 "
    	"-Wall -Wextra -Wpedantic "
		"-Wconversion -Wshadow -Wformat=2 "
    	"-fsanitize=address,undefined "
    	"game.c -o \"Space Shooter\" "
	    "-lraylib -lm";

const char *raylib_link = "https://github.com/raysan5/raylib/archive/refs/tags/6.0.zip";

char src[1000] = {0};

void fetchRaylib() {
    char cmd[2048];
    
    printf("[*] Downloading Raylib...\n");
    snprintf(cmd, sizeof(cmd), "wget -O raylib-6.0.zip %s", raylib_link);
    system(cmd);

    printf("[*] Extracting and Building Raylib (this may take a minute)...\n");
    
    const char *build_steps = 
        "mkdir -p raylib && "
        "unzip -q raylib-6.0.zip && "
        "cd raylib-6.0 && "
        "mkdir -p build && "
        "cd build && "
        "cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=../../raylib .. && "
        "cmake --build . --target install && "
        "cd ../.. && "
        "rm -rf raylib-6.0 raylib-6.0.zip";

    system(build_steps);
    printf("[+] Raylib built locally inside ./raylib/\n");
}

int main(int argc, char **argv) {
    // Check if local Raylib exists
    if ((access("raylib/include", F_OK) == 0) && (access("raylib/lib", F_OK) == 0)) {
        // Added rpath so the executable knows where to find the .so at runtime
        snprintf(src, sizeof(src), "-Iraylib/include -Lraylib/lib -Wl,-rpath,./raylib/lib");
    }
    
    if (argc < 2 || strcmp(argv[1], "--release") == 0) {
        char cmd[3000];
        snprintf(cmd, sizeof(cmd), "%s %s", release_build, src);
        printf("Running: %s\n", cmd);
        return system(cmd);
    }

    if (strcmp(argv[1], "--debug") == 0) {
        char cmd[3000];
        snprintf(cmd, sizeof(cmd), "%s %s", debug_build, src);
        printf("Running: %s\n", cmd);
        return system(cmd);
    }

    if (strcmp(argv[1], "--help") == 0) {
        puts("HELP MENU:");
        puts("   CMD                    ---                 Action");
        puts("./build                   ---              Release build");
        puts("./build --debug           ---              Debug Build");
        puts("./build --download <dep>  ---              Downloads and links locally (Not System-Wide)");
        return 0;
    }
    
    if (strcmp(argv[1], "--download") == 0) {
        if (argc < 3) {
            puts("Usage: ./build --download <dep>");
            return 1;
        }

        if (strcmp(argv[2], "raylib") == 0) {
            fetchRaylib();
            return 0;
        }
    }

    puts("INCORRECT USAGE\nTry \"./build --help\"\n");
    return 1;
}
