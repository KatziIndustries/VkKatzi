#ifndef VKKATZI_GLFW_H
#define VKKATZI_GLFW_H

#include "vkkatzi.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

VKK_Result VKK_CreateSurfaceGLFW(GLFWwindow* window, VKK_Instance instance, VKK_Surface* o_surface);

#ifdef __cplusplus
}
#endif

#endif