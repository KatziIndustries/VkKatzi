#ifndef VKKATZI_SDL_H
#define VKKATZI_SDL_H

#include "vkkatzi.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

VKK_Result VKK_CreateSurfaceSDL(SDL_Window* window, VKK_Instance instance, VKK_Surface* o_surface);

#ifdef __cplusplus
}
#endif

#endif
