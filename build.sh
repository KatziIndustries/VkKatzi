#/usr/bin/env bash

shader/recompile_shaders.sh
gcc main.c src/* logger.c -lvulkan -lm -lSDL3 -o main