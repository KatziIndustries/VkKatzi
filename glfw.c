#include "include/vkkatzi.h"
#include "include/vkkatzi_glfw.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

VKK_Result VKK_CreateSurfaceGLFW(GLFWwindow* window, VKK_Instance instance, VKK_Surface* o_surface) {
    
    VkInstance vkInstance = _VKK_Internal_GetRawInstanceHandle(instance);

    VkSurfaceKHR surface;
    glfwCreateWindowSurface(vkInstance, window, NULL, &surface);

    if (!surface) {
        return VKK_ERROR_SURFACE_CREATION_FAILED;
    }

    VKK_Surface wrappedSurface = _VKK_Internal_WrapSurface(surface);
    *o_surface = wrappedSurface;

    return VKK_SUCCESS;
}