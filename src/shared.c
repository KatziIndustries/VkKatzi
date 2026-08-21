#include "../include/vkcontext.h"
#include "../include/logger.h"

#include <vulkan/vulkan.h>

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vkContext.physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
    }

    LogError("Failed to find suitable memory type");
    return UINT32_MAX;
}
