#/usr/bin/env bash

shader/recompile_shaders.sh
gcc src/vulkan.c main.c logger.c src/sdl.c -lvulkan -lm -lSDL3 -o main