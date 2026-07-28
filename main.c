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
        .presentMode = VKK_PRESENT_MODE_IMMEDIATE,
        .imageBufferSize = 3,
        .enableValidationLayers = true,
        .verboseLogging = true
    };

    if (!VKK_InitDevice(window, config)) {
        fprintf(stderr, "Failed to initialize Vulkan context\n");
        exit(1);
    }

    VKK_Uniform timeUniform = VKK_CreateUniform(0, sizeof(float), VKK_SHADER_STAGE_VERTEX);
    VKK_Uniform positionUniform = VKK_CreateUniform(32, sizeof(float) * 2, VKK_SHADER_STAGE_FRAGMENT);

    if (!VKK_InitPipeline()) {
        fprintf(stderr, "Failed to initialize pipeline\n");
        exit(1);
    }


    static const VKK_Vertex vertices[] = {
        {{0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
        {{1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        {{-1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    };

    static const uint16_t indices[] = {
        0, 1, 2, 
    };

    VKK_Buffer vertexBuffer = VKK_CreateBuffer(sizeof(VKK_Vertex) * 3, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(vertexBuffer, vertices, sizeof(VKK_Vertex) * 3, 0);

    VKK_Buffer indexBuffer = VKK_CreateBuffer(sizeof(uint16_t) * 3, VKK_BUFFER_USAGE_INDEX);
    VKK_WriteBuffer(indexBuffer, indices, sizeof(uint16_t) * 3, 0);

    float elapsedTime = 0;
    float elapsedTotal = 0;
    
    double lastFrameTime = glfwGetTime();

    while (!VKK_WindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        elapsedTime += deltaTime;
        elapsedTotal += deltaTime;

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

        double mousePos[2];

        glfwGetCursorPos(window, &mousePos[0], &mousePos[1]);
        
        float mousePosF[2] = {
            (float)mousePos[0],
            (float)mousePos[1]            
        };
        VKK_WriteUniform(positionUniform, mousePosF, sizeof(float) * 2, 0);
        
        VKK_SetMousePosition(mousePos[0], mousePos[1]);
        
        //VKK_WriteUniform(timeUniform, &elapsedTotal, sizeof(float), 0);

        VKK_Draw(vertexBuffer, 3, indexBuffer, 3);

	    VKK_PollEvents();
        VKK_Present();
    }

    VKK_DestroyBuffer(vertexBuffer);
    VKK_DestroyBuffer(indexBuffer);

    VKK_DestroyUniform(timeUniform);
    VKK_DestroyUniform(positionUniform);

    VKK_End();
    VKK_TerminateWindowing();
}
