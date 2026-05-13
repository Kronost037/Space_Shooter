main: game.c
	gcc -Wall -Wextra game.c -o game -lraylib && ./game
