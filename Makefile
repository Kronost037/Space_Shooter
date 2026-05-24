main: game.c
	gcc -std=c17 -O0 -g3 \
    	-Wall -Wextra -Wpedantic \
    	-Wconversion -Wshadow -Wformat=2 \
    	-fsanitize=address,undefined \
    	game.c -o "Space Shooter" \
    	-lraylib -lm
