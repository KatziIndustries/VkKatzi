#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "include/window.h"
#include "include/shared.h"
#include "include/vkKatzi.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

int lastWindowWidth = WINDOW_WIDTH;
int lastWindowHeight = WINDOW_HEIGHT;

GLFWwindow* window;

bool leftMousePressed;


float* mergeArrays(float arr1[], int n1, float arr2[], int n2) {
  	float *res = (float*)malloc(sizeof(float) * (n1 + n2));
    memcpy(res, arr1, n1 * sizeof(float));
    memcpy(res + n1, arr2, n2 * sizeof(float));
  	return res;
}

static void CreateOrthoMatrix(float* o_matrix, float width, float height) {

    for (int i = 0; i < 16; i++)   
        o_matrix[i] = 0.0f;
    
    o_matrix[0] = 2.0f / width;
    o_matrix[5] = 2.0f / height;
    o_matrix[10] = 1.0f;
    o_matrix[12] = -1.0f;
    o_matrix[13] = -1.0f;
    o_matrix[15] = 1.0f;
}

int main() {

    window = VKK_CreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Katzi lol");

    VKK_Config config = {
        .presentMode = VKK_PRESENT_MODE_IMMEDIATE,
        .imageBufferSize = 3,
        .enableValidationLayers = true,
        .verboseLogging = true
    };

    VKK_PhysicalDeviceInfo deviceInfo = VKK_InitDevice(window, config);

    if (!deviceInfo.success) {
        fprintf(stderr, "Failed to initialize Vulkan context\n");
        exit(1);
    }

    fprintf(stdout, "[Device]: Name: %s, Api Version: %i, Device ID: %i, Device Type: %i, Driver Version: %i, Vendor ID: %i\n", deviceInfo.name, deviceInfo.apiVersion, deviceInfo.deviceID, deviceInfo.deviceType, deviceInfo.driverVersion, deviceInfo.vendorID);

    VKK_Uniform timeUniform = VKK_CreateUniform(0, sizeof(float), VKK_SHADER_STAGE_VERTEX);

    VKK_PushConstantRange pushConstantRange = {
        .shaderStage = VKK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = sizeof(float) * 20
    };

    if (!VKK_InitPipeline(pushConstantRange)) {
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
    
    double lastFrameTime = VKK_GetTime();

    while (!VKK_WindowShouldClose(window))
    {
        double currentTime = VKK_GetTime();
        double deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        elapsedTime += deltaTime;
        elapsedTotal += deltaTime;

        if (elapsedTime > 1) {
            fprintf(stdout, "Frametime: %lf, FPS: %lf\n", deltaTime, 1.0 / deltaTime);
            fflush(stdout);
            elapsedTime = 0;
        }

        int pressed = VKK_GetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

        if (pressed == 1 && !leftMousePressed) {
            leftMousePressed = true;
        }

        if (pressed == 0) {
            leftMousePressed = false;
        }

        double mousePos[2];
        VKK_GetCursorPosition(window, &mousePos[0], &mousePos[1]);
        
        float mousePosF[2] = {
            (float)mousePos[0],
            (float)mousePos[1]
        };

        int windowWidth;
        int windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        float matrix[16];
        CreateOrthoMatrix(matrix, windowWidth, windowHeight);

        float* pushData = mergeArrays(matrix, 16, mousePosF, 2);

        VKK_SetPushConstantData(pushData);

        VKK_Draw(vertexBuffer, 3, indexBuffer, 3);

	    VKK_PollEvents();
        VKK_Present();

        free(pushData);
    }

    VKK_DestroyBuffer(vertexBuffer);
    VKK_DestroyBuffer(indexBuffer);

    VKK_DestroyUniform(timeUniform);

    VKK_End();
    VKK_TerminateWindowing();
}