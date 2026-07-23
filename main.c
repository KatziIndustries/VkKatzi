#include <stdio.h>
#include <stdlib.h>

#include "include/vulkan.h"
#include "include/window.h"
#include "include/shared.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

int lastWindowWidth = WINDOW_WIDTH;
int lastWindowHeight = WINDOW_HEIGHT;

GLFWwindow* window;

bool leftMousePressed;

int main() {

    window = VKK_CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Katzi lol");

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

    double elapsedTime = 0;
    double lastFrameTime = glfwGetTime();

    while (!VKK_WindowShouldClose(window))
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
            rect.x = mouseX;
            rect.y = mouseY;
        }

        if (pressed == 0) {
            leftMousePressed = false;
        }

	    VKK_PollEvents();
        VKK_Present();
    }

    VKK_End();
    VKK_TerminateWindowing();
}
