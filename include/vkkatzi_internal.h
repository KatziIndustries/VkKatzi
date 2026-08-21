#ifndef VKKATZI_INTERNAL_H
#define VKKATZI_INTERNAL_H

#include "vkkatzi.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

struct VKK_Instance_T {
    VkInstance handle;
};

struct VKK_Surface_T {
    VkSurfaceKHR handle;
};

VkInstance _VKK_Internal_GetRawInstanceHandle(VKK_Instance instance);
VKK_Surface _VKK_Internal_WrapSurface(VkSurfaceKHR rawSurface);

#ifdef __cplusplus
}
#endif

#endif