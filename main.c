#include <stdio.h>
#include <stdlib.h>

#include "include/vulkan.h"
#include "include/shared.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

int lastWindowWidth = WINDOW_WIDTH;
int lastWindowHeight = WINDOW_HEIGHT;

GLFWwindow* window;

bool leftMousePressed;

int main() {

    if (!glfwInit())
    return -1;
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Katzi lol", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    
    if (!VKK_Init(window)) {
        fprintf(stderr, "Failed to initialize Vulkan context\n");
        exit(1);
    }

    VKK_Rectangle rect = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100
    };

    VKK_AddRectangle(rect);

    double elapsedTime = 0;
    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
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

        int pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

        if (pressed == 1 && !leftMousePressed) {
            leftMousePressed = true;
            
            double mouseX;
            double mouseY;

            glfwGetCursorPos(window, &mouseX, &mouseY);

            VKK_Rectangle rect = {
                .x = mouseX,
                .y = mouseY,
                .width = 50,
                .height = 50
            };

            VKK_AddRectangle(rect);
        }

        if (pressed == 0) {
            leftMousePressed = false;
        }

        glfwPollEvents();
        VKK_Present();
    }

    VKK_End();
    glfwTerminate();
}
