#/usr/bin/env bash

shader/recompile_shaders.sh
gcc main.c vulkan.c window.c -lvulkan -lglfw -o main