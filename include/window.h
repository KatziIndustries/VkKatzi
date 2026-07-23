#ifndef VKK_WINDOW_H
#define VKK_WINDOW_H

#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>

GLFWwindow* VKK_CreateWindow(int width, int height, char* title);
bool VKK_WindowShouldClose(GLFWwindow* window);
void VKK_TerminateWindowing();
void VKK_PollEvents();

#endif
