#include "../include/vkcontext.h"
#include "../include/logger.h"
#include "../include/shared.h"

#include <vulkan/vulkan.h>

bool logWarnings;

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

VkShaderStageFlags ConvertShaderStage(VKK_ShaderStage shaderStage) {
    switch (shaderStage) {
        case VKK_SHADER_STAGE_VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;

        case VKK_SHADER_STAGE_FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;

        case VKK_SHADER_STAGE_ALL:
            return VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    return VK_SHADER_STAGE_VERTEX_BIT;
}
