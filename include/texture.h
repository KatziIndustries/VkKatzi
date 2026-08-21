#ifndef TEXTURE_H
#define TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/vkkatzi.h"

#include <vulkan/vulkan.h>

struct VKK_Texture_T {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkSampler sampler;
    uint32_t width, height;
};

void DestroyTexture(VKK_Texture texture);
bool CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage* o_image, VkDeviceMemory* o_memory);

#ifdef __cplusplus
}
#endif

#endif