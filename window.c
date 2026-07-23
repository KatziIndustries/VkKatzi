#include <stdio.h>
#include <stdbool.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

GLFWwindow* VKK_CreateWindow(int width, int height, char* title) {

    GLFWwindow* window;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return NULL;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(width, height, title, NULL, NULL);

    if (!window)
    {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return NULL;
    }

    return window;
}

bool VKK_WindowShouldClose(GLFWwindow* window) {
    return glfwWindowShouldClose(window);
}

void VKK_TerminateWindowing() {
    glfwTerminate();
}

void VKK_PollEvents() {
    glfwPollEvents();
}
