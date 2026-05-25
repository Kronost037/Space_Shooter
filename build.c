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

const char *raylib_link = "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz";

char src[1000] = {0};

int main(int argc, char **argv) {

    if((access("raylib/include", F_OK) == 0) && (access("raylib/lib", F_OK) == 0)) {
        strcat(src, " -Iraylib/include -Lraylib/lib");
    }
    
    if(argc < 2 || strcmp(argv[1], "--release") == 0) {
        char cmd[1000] = {0};
        strcat(cmd, release_build);
        strcat(cmd, src);
		system(cmd);
		return 0;
	}

	if(strcmp(argv[1], "--debug") == 0) {
		char cmd[1000] = {0};
        strcat(cmd, debug_build);
        strcat(cmd, src);
		system(cmd);
		return 0;
	}

	if(strcmp(argv[1], "--help") == 0) {
		puts("HELP MENU:");
		puts("   CMD                    ---                 Action");
		puts("./build                   ---              Release build");
		puts("./build --debug           ---              Debug Build");
        puts("./build --download <dep>  ---              Downloads and links locally (Not System-Wide)");
		return 0;
	}
    
	if(strcmp(argv[1], "--download") == 0) {
		if(argc < 3) {
			puts("Usage: ./build --download <dep>");
			return 0;
		}

		if(strcmp(argv[2], "raylib") == 0) {
            char cmd[1000] = "wget ";
            strcat(cmd, raylib_link);
			system(cmd);

			system("mkdir raylib && tar -zxvf raylib-5.5_linux_amd64.tar.gz -C raylib --strip-components=1");

            system("rm raylib-5.5_linux_amd64.tar.gz");
            return 0;
		}
	}

    puts("INCORRECT USAGE");
	puts("\nTry \"./build --help\"\n");

	return 0;
}
