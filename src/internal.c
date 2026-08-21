#include "../include/vkkatzi.h"
#include "../include/vkkatzi_internal.h"

#include <vulkan/vulkan.h>

VkInstance _VKK_Internal_GetRawInstanceHandle(VKK_Instance instance) {
    return instance->handle;
}

VKK_Surface _VKK_Internal_WrapSurface(VkSurfaceKHR rawSurface) {
    VKK_Surface surface = malloc(sizeof(struct VKK_Surface_T));
    surface->handle = rawSurface;
    return surface;
}