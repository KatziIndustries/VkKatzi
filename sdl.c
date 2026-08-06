#include "include/vkkatzi.h"
#include "include/vkkatzi_sdl.h"
#include "include/vkkatzi_internal.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

VKK_Result VKK_CreateSurfaceSDL(SDL_Window* window, VKK_Instance instance, VKK_Surface* o_surface) {
    
    VkInstance vkInstance = _VKK_Internal_GetRawInstanceHandle(instance);

    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(window, vkInstance, NULL, &surface);

    if (!surface) {
        return VKK_ERROR_SURFACE_CREATION_FAILED;
    }

    VKK_Surface wrappedSurface = _VKK_Internal_WrapSurface(surface);
    *o_surface = wrappedSurface;

    return VKK_SUCCESS;
}