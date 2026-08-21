#ifndef BUFFER_H
#define BUFFER_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../include/vkkatzi.h"

#include <vulkan/vulkan.h>
#include <stdbool.h>

struct VKK_Buffer_T {
    VkBuffer handle;
    VkDeviceMemory memory;
    VkDeviceSize size;
    bool isMapped;
    void* mappedPtr;
};

void DestroyBuffer(VKK_Buffer buffer);

#ifdef __cplusplus
}
#endif

#endif