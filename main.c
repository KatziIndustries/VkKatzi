#include <stdio.h>
#include <stdbool.h>

#include "include/window.h"
#include "include/shared.h"
#include "include/vkKatzi.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

int lastWindowWidth = WINDOW_WIDTH;
int lastWindowHeight = WINDOW_HEIGHT;

GLFWwindow* window;

bool leftMousePressed;

bool e;

int main() {

    window = VKK_CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Katzi lol");

    VKK_Config config = {
        .presentMode = VKK_PRESENT_MODE_FIFO,
        .imageBufferSize = 3,
        .enableValidationLayers = true,
        .verboseLogging = true
    };

    VKK_InitInfo initInfo = VKK_Init(window, config);

    if (!initInfo.success) {
        fprintf(stderr, "Failed to initialize Vulkan context\n");
        exit(1);
    }

    fprintf(stdout, "[GPU] Name: %s, Api Version: %i, Driver version: %i\n", initInfo.deviceInfo.name, initInfo.deviceInfo.apiVersion, initInfo.deviceInfo.driverVersion);
    fprintf(stdout, "[Present mode] %s\n", initInfo.presentModeString);
    fprintf(stdout, "[Image Buffer]: %i (Max: %i, Min: %i)\n", initInfo.imageBufferSize, initInfo.maxImageBufferSize, initInfo.minImageBufferSize);

    //static const Vertex vertices[] = {
    //    {{0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    //    {{1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    //    {{-1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},

    //    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    //    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    //    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    //    {{-0.5f, 0.5f}, {0.0f, 0.0f, 0.0f, 1.0f}},
    //};

    static const VKK_Vertex vertices[] = {
        {{960.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1920.0f, 1080.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{0, 1080.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    };

    static const uint16_t indices[] = {
        0, 1, 2, 
    };

    VKK_Buffer vertexBuffer = VKK_CreateBuffer(sizeof(VKK_Vertex) * 3, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(vertexBuffer, vertices, sizeof(VKK_Vertex) * 3, 0);

    VKK_Buffer indexBuffer = VKK_CreateBuffer(sizeof(uint16_t) * 3, VKK_BUFFER_USAGE_INDEX);
    VKK_WriteBuffer(indexBuffer, indices, sizeof(uint16_t) * 3, 0);

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
            
        }

        if (pressed == 0) {
            leftMousePressed = false;
        }

        double mouseX;
        double mouseY;

        glfwGetCursorPos(window, &mouseX, &mouseY);

        VKK_SetMousePosition(mouseX, mouseY);
        VKK_Draw(vertexBuffer, 3, indexBuffer, 3);

	    VKK_PollEvents();
        VKK_Present();
    }

    VKK_DestroyBuffer(vertexBuffer);
    VKK_DestroyBuffer(indexBuffer);

    VKK_End();
    VKK_TerminateWindowing();
}
