#ifndef VKKATZI_H
#define VKKATZI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VKK_Buffer_T* VKK_Buffer;
typedef struct VKK_Uniform_T* VKK_Uniform;

typedef enum {
    VKK_BUFFER_USAGE_VERTEX,
    VKK_BUFFER_USAGE_INDEX,
    VKK_BUFFER_USAGE_UNIFORM,
    VKK_BUFFER_USAGE_STORAGE,
    VKK_BUFFER_USAGE_STAGING,
} VKK_BufferUsage;

typedef enum {
    VKK_PRESENT_MODE_IMMEDIATE = 0,
    VKK_PRESENT_MODE_MAILBOX = 1,
    VKK_PRESENT_MODE_FIFO = 2,
    VKK_PRESENT_MODE_FIFO_RELAXED = 3,
    VKK_PRESENT_MODE_SHARED_DEMAND_REFRESH = 1000111000,
    VKK_PRESENT_MODE_SHARED_CONTINOUS_REFRESH = 1000111001,
    VKK_PRESENT_MODE_FIFO_LATEST_READY = 1000361000
} VKK_PresentMode;

typedef enum {
    VKK_SHADER_STAGE_VERTEX,
    VKK_SHADER_STAGE_FRAGMENT,
    VKK_SHADER_STAGE_ALL,
} VKK_ShaderStage;

typedef struct {
    float position[2];
    float color[4];
} VKK_Vertex;

typedef struct {
    VKK_PresentMode presentMode;
    uint32_t imageBufferSize;
    bool enableValidationLayers;
    bool verboseLogging;
} VKK_Config;

typedef struct {
    char name[256];
    uint32_t apiVersion;
    uint32_t driverVersion;
} VKK_PhysicalDeviceInfo;

bool VKK_InitDevice(GLFWwindow* window, VKK_Config config);
bool VKK_InitPipeline(void);
void VKK_End(void);

void VKK_Present(void);

VKK_Buffer VKK_CreateBuffer(size_t size, VKK_BufferUsage usage);
void VKK_DestroyBuffer(VKK_Buffer buffer);
void VKK_WriteBuffer(VKK_Buffer buffer, const void* data, size_t size, size_t offset);

VKK_Uniform VKK_CreateUniform(uint32_t binding, size_t size, VKK_ShaderStage shaderStage);
void VKK_WriteUniform(VKK_Uniform uniform, const void* data, size_t size, size_t offset);
void VKK_DestroyUniform(VKK_Uniform uniform);
void VKK_BindUniform(VKK_Uniform uniform);

void VKK_Draw(VKK_Buffer vertexBuffer, uint32_t vertexCount, VKK_Buffer indexBuffer, uint32_t indexCount);

void VKK_SetMousePosition(float x, float y);

#ifdef __cplusplus
}
#endif

#endif