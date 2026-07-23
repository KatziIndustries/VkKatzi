#ifndef VULKAN_H
#define VULKAN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "rectangle.h"

#define MAX_FRAMES_IN_FLIGHT 2

void VKK_Present();
bool VKK_Init(GLFWwindow* window);
void VKK_AddRectangle(VKK_Rectangle rectangle);
void VKK_End();

#endif
