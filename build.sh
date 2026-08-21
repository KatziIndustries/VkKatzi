#/usr/bin/env bash

shader/recompile_shaders.sh
gcc main.c src/* logger.c sdl.c -lvulkan -lm -lSDL3 -o main