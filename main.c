#include <stdio.h>
#include <stdlib.h>

#include "vulkan.h"
#include "shared.h"

const char PathSeparator =
#ifdef _WIN32
    '\\';
#else
    '/';
#endif


const uint32_t WINDOW_WIDTH = 1080;
const uint32_t WINDOW_HEIGHT = 720;

int main() {

    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Katzi lol", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    VkContext context;
    InitVkContext(&context, window);

    double elapsedTime = 0;
    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(context.window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        elapsedTime += deltaTime;

        if (elapsedTime > 1) {
            fprintf(stdout, "Frametime: %lf, FPS: %lf\n", deltaTime, 1.0 / deltaTime);
            fflush(stdout);
            elapsedTime = 0;
        }

        glfwPollEvents();
        DrawFrame(&context);
    }

    vkDeviceWaitIdle(context.logicalDevice);
    glfwTerminate();
}