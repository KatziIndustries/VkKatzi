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
typedef struct VKK_Pipeline_T* VKK_Pipeline;

typedef struct {
    const char* vertexShaderPath; 
    const char* fragmentShaderPath; 
} VKK_ShaderPaths;

typedef enum {
    VKK_VERTEX_FORMAT_FLOAT2,
    VKK_VERTEX_FORMAT_FLOAT3,
    VKK_VERTEX_FORMAT_FLOAT4
} VKK_VertexFormat;

typedef struct {
    uint32_t location;
    VKK_VertexFormat format;
    uint32_t offset;
} VKK_VertexAttribute;

typedef struct {
    VKK_ShaderPaths shaderPaths;
    VKK_VertexAttribute* attributes;
    uint32_t attributeCount;
    uint32_t vertexStride;
} VKK_PipelineDescription;

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

typedef enum {
    VKK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
    VKK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
    VKK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VKK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
    VKK_PHYSICAL_DEVICE_TYPE_CPU = 4,
    VKK_PHYSICAL_DEVICE_TYPE_MAX_ENUM = 0x7FFFFFFF
} VKK_PhysicalDeviceType;

typedef struct {
    VKK_PresentMode presentMode;
    uint32_t imageBufferSize;
    bool enableValidationLayers;
    bool verboseLogging;
} VKK_Config;

typedef struct {
    bool success;
    char name[256];
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t deviceID;
    VKK_PhysicalDeviceType deviceType;
    uint32_t vendorID;
} VKK_PhysicalDeviceInfo;

typedef struct {
    VKK_ShaderStage shaderStage;
    uint32_t offset;
    uint32_t size;
} VKK_PushConstantRange;

VKK_PhysicalDeviceInfo VKK_InitDevice(GLFWwindow* window, VKK_Config config);
bool VKK_InitPipeline();
void VKK_End(void);

void VKK_Present(void);

VKK_Buffer VKK_CreateBuffer(size_t size, VKK_BufferUsage usage);
void VKK_DestroyBuffer(VKK_Buffer buffer);
void VKK_WriteBuffer(VKK_Buffer buffer, const void* data, size_t size, size_t offset);

VKK_Uniform VKK_CreateUniform(uint32_t binding, size_t size, VKK_ShaderStage shaderStage);
void VKK_WriteUniform(VKK_Uniform uniform, const void* data, size_t size, size_t offset);
void VKK_DestroyUniform(VKK_Uniform uniform);

void VKK_Draw(VKK_Pipeline pipeline, VKK_Buffer vertexBuffer, uint32_t vertexCount, VKK_Buffer indexBuffer, uint32_t indexCount);

void VKK_SetPushConstantData(void* data);

VKK_Pipeline VKK_CreatePipeline(VKK_PipelineDescription desc, VKK_PushConstantRange pushConstanRangeConfig);
void VKK_DestroyPipeline(VKK_Pipeline pipeline);

GLFWwindow* VKK_CreateWindow(int width, int height, char* title);
bool VKK_WindowShouldClose(GLFWwindow* window);
void VKK_TerminateWindowing();
void VKK_PollEvents();
double VKK_GetTime();
int VKK_GetMouseButton(GLFWwindow* window, int button);
void VKK_GetCursorPosition(GLFWwindow* window, double* x, double* y);
void VKK_GetFramebufferSize(GLFWwindow* window, int* width, int* height);

#ifdef __cplusplus
}
#endif

#endif
