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

typedef enum {
    VKK_BUFFER_USAGE_VERTEX,
    VKK_BUFFER_USAGE_INDEX,
    VKK_BUFFER_USAGE_UNIFORM,
    VKK_BUFFER_USAGE_STORAGE,
    VKK_BUFFER_USAGE_STAGING,
} VKK_BufferUsage;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} VKK_Rectangle;

typedef struct {
    float position[2];
    float color[4];
} Vertex;

bool VKK_Init(GLFWwindow* window);
void VKK_End(void);

void VKK_Present(void);
void VKK_RenderRectangle(VKK_Rectangle rectangle);

VKK_Buffer VKK_CreateBuffer(size_t size, VKK_BufferUsage usage);
void VKK_DestroyBuffer(VKK_Buffer buffer);
void VKK_WriteBuffer(VKK_Buffer buffer, const void* data, size_t size, size_t offset);

void VKK_Draw(VKK_Buffer vertexBuffer, uint32_t vertexCount, VKK_Buffer indexBuffer, uint32_t indexCount);

#ifdef __cplusplus
}
#endif

#endif