#include "../include/vkkatzi.h"
#include "../include/texture.h"
#include "../include/buffer.h"
#include "../include/logger.h"
#include "../include/shared.h"
#include "../include/vkcontext.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#include <vulkan/vulkan.h>

void VKK_BindTexture(uint32_t binding, VKK_Texture texture) {

    const VkDescriptorImageInfo imageInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = texture->imageView,
        .sampler = texture->sampler,
    };

    const VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkContext.descriptorSet,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(vkContext.logicalDevice, 1, &descriptorWrite, 0, NULL);
}

static VkCommandBuffer BeginSingleTimeCommands() {

    const VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = vkContext.commandPool,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vkContext.logicalDevice, &allocateInfo, &commandBuffer);

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void EndSingleTimeCommands(VkCommandBuffer commandBuffer) {

    vkEndCommandBuffer(commandBuffer);

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer   
    };

    vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkContext.graphicsQueue);

    vkFreeCommandBuffers(vkContext.logicalDevice, vkContext.commandPool, 1, &commandBuffer);
}

static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    const VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,   
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(commandBuffer);
}

static void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {

    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags sourceStage, destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        LogError("TransitionImageLayout: unsupported layout transition");
        EndSingleTimeCommands(commandBuffer);
        return;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);

    EndSingleTimeCommands(commandBuffer);
}

bool CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage* o_image, VkDeviceMemory* o_memory) {

    const VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent = { width, height, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    assert(vkContext.logicalDevice != VK_NULL_HANDLE);

    if (vkCreateImage(vkContext.logicalDevice, &imageInfo, NULL, o_image) != VK_SUCCESS) {
        LogError("Failed to create Image");
        return false;
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(vkContext.logicalDevice, *o_image, &memoryRequirements);

    uint32_t memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    const VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex   
    };

    if (vkAllocateMemory(vkContext.logicalDevice, &allocateInfo, NULL, o_memory) != VK_SUCCESS) {
        LogError("Failed to allocate image memory");
        return false;
    }

    vkBindImageMemory(vkContext.logicalDevice, *o_image, *o_memory, 0);

    return true;
}

VKK_Texture VKK_CreateTextureFromPixels(const void* pixels, uint32_t width, uint32_t height, VKK_Format textureFormat) {

    if (!vkContext.commandPool) {
        LogError("VKK_InitRenderer has to be called before creating a Texture");
        return NULL;
    }
    
    VkDeviceSize imageSize = (VkDeviceSize)width * height * 4;

    VKK_Buffer stagingBuffer = VKK_CreateBuffer(imageSize, VKK_BUFFER_USAGE_STAGING);
    VKK_WriteBuffer(stagingBuffer, pixels, imageSize, 0);

    VKK_Texture texture = malloc(sizeof(struct VKK_Texture_T));
    texture->width = width;
    texture->height = height;
    
    if (!CreateImage(width, height, (VkFormat)textureFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &texture->image, &texture->memory)) {
        free(texture);
        VKK_DestroyBuffer(stagingBuffer);
        return NULL;
    }
    
    TransitionImageLayout(texture->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    CopyBufferToImage(stagingBuffer->handle, texture->image, width, height);
    
    TransitionImageLayout(texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    VKK_DestroyBuffer(stagingBuffer);

    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = texture->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = (VkFormat)textureFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1
        },
    };

    if (vkCreateImageView(vkContext.logicalDevice, &viewInfo, NULL, &texture->imageView) != VK_SUCCESS) {
        LogError("Failed to create texture image view");
        free(texture);
        return NULL;
    }

    const VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = (VkFilter)VKK_SAMPLER_FILTER_LINEAR,
        .minFilter = (VkFilter)VKK_SAMPLER_FILTER_LINEAR,
        .addressModeU = (VkSamplerAddressMode)VKK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = (VkSamplerAddressMode)VKK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = (VkSamplerAddressMode)VKK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_FALSE,
        .borderColor = (VkBorderColor)VKK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    };

    if (vkCreateSampler(vkContext.logicalDevice, &samplerInfo, NULL, &texture->sampler) != VK_SUCCESS) {
        LogError("Failed to create texture sampler");
        free(texture);
        return NULL;
    }

    return texture;
}

VKK_Texture VKK_CreateTexture(const char* path, VKK_Format textureFormat) {

    int width, height, channels;

    stbi_uc* pixels = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels) {
        fprintf(stderr, "[VKK][ERROR]: Failed to load image '%s'\n", path);
        return NULL;
    }
    
    VKK_Texture texture = VKK_CreateTextureFromPixels(pixels, (uint32_t)width, (uint32_t)height, textureFormat);

    stbi_image_free(pixels);

    return texture;
}


void VKK_GetTextureSize(VKK_Texture texture, uint32_t* o_width, uint32_t* o_height) {
    *o_width = texture->width;
    *o_height = texture->height;
}

void VKK_SetTextureSampler(VKK_Texture texture, VKK_SamplerInfo samplerInfo) {

    const VkSamplerCreateInfo vkSamplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = (VkFilter)samplerInfo.filter,
        .minFilter = (VkFilter)samplerInfo.filter,
        .addressModeU = (VkSamplerAddressMode)samplerInfo.addressMode,
        .addressModeV = (VkSamplerAddressMode)samplerInfo.addressMode,
        .addressModeW = (VkSamplerAddressMode)samplerInfo.addressMode,
        .anisotropyEnable = samplerInfo.enableAnisotropy,
        .maxAnisotropy = samplerInfo.maxAnisotropy == 0.0f ? 1.0f : samplerInfo.maxAnisotropy,
        .borderColor = (VkBorderColor)samplerInfo.borderColor,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    };

    vkDestroySampler(vkContext.logicalDevice, texture->sampler, NULL);

    if (vkCreateSampler(vkContext.logicalDevice, &vkSamplerInfo, NULL, &texture->sampler) != VK_SUCCESS) {
        LogError("Failed to create texture sampler");
        VKK_DestroyTexture(texture);
    }
}

void VKK_DestroyTexture(VKK_Texture texture) {

    if (!texture)
        return;

    PendingDeletion deletion = {
        .deletionType = DELETION_TEXTURE,
        .framesUntilDeletion = MAX_FRAMES_IN_FLIGHT,
        .texture = texture   
    };

    vkContext.pendingDeletions[vkContext.pendingDeletionCount++] = deletion;
}

void DestroyTexture(VKK_Texture texture) {

    if (!texture)
        return;

    vkDestroySampler(vkContext.logicalDevice, texture->sampler, NULL);
    vkDestroyImageView(vkContext.logicalDevice, texture->imageView, NULL);
    vkDestroyImage(vkContext.logicalDevice, texture->image, NULL);
    vkFreeMemory(vkContext.logicalDevice, texture->memory, NULL);
    free(texture);
}