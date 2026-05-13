main: game.c
	gcc -Wall -Wextra -Idependencies game.c -o "Space Shooter" -Ldependencies -lraylib -lm
