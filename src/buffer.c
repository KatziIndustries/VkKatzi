#include "../include/buffer.h"
#include "../include/vkkatzi.h"
#include "../include/vkcontext.h"
#include "../include/logger.h"
#include "../include/shared.h"

#include <string.h>

static void GetBufferUsageFlags(VKK_BufferUsage usage, VkBufferUsageFlags* o_usageFlags, VkMemoryPropertyFlags* o_memoryFlags) {
    switch (usage) {
        case VKK_BUFFER_USAGE_VERTEX:
            *o_usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            *o_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case VKK_BUFFER_USAGE_INDEX:
            *o_usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            *o_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case VKK_BUFFER_USAGE_UNIFORM:
            *o_usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            *o_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case VKK_BUFFER_USAGE_STORAGE:
            *o_usageFlags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            *o_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;

        case VKK_BUFFER_USAGE_STAGING:
            *o_usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            *o_memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            break;
    }
}

static bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* o_buffer, VkDeviceMemory* o_bufferMemory) {
    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(vkContext.logicalDevice, &bufferInfo, NULL, o_buffer) != VK_SUCCESS) {
        LogError("Failed to create buffer");
        return false;
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(vkContext.logicalDevice, *o_buffer, &memoryRequirements);

    uint32_t memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (memoryTypeIndex == UINT32_MAX) {
        return false;
    }

    const VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    if (vkAllocateMemory(vkContext.logicalDevice, &allocateInfo, NULL, o_bufferMemory)) {
        LogError("Failed to allocate buffer memory");
        return false;
    }

    vkBindBufferMemory(vkContext.logicalDevice, *o_buffer, *o_bufferMemory, 0);

    return true;
}

VKK_Buffer VKK_CreateBuffer(size_t size, VKK_BufferUsage usage) {

    VkBufferUsageFlags usageFlags;
    VkMemoryPropertyFlags memoryFlags;
    GetBufferUsageFlags(usage, &usageFlags, &memoryFlags);

    VKK_Buffer buffer = malloc(sizeof(struct VKK_Buffer_T));
    buffer->size = (VkDeviceSize)size;
    buffer->isMapped = false;
    buffer->mappedPtr = NULL;

    if (!CreateBuffer(buffer->size, usageFlags, memoryFlags, &buffer->handle, &buffer->memory)) {
        LogError("VKK_CreateBuffer: Failed to create buffer");
        free(buffer);
        return NULL;
    }

    return buffer;
}

void VKK_WriteBuffer(VKK_Buffer buffer, const void* data, size_t size, size_t offset) {

    if (!buffer) {
        LogError("VKK_WriteBuffer: buffer is NULL");
        return;
    }

    if (offset + size > buffer->size)  {
        fprintf(stderr, "[VKK][ERROR]: VKK_WriteBuffer: write out of bounds (offset %zu + size %zu > buffer size %llu)\n", offset, size, (unsigned long int)buffer->size);
        return;
    }

    void* mapped;
    vkMapMemory(vkContext.logicalDevice, buffer->memory, offset, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(vkContext.logicalDevice, buffer->memory);
}

void DestroyBuffer(VKK_Buffer buffer) {
    vkDestroyBuffer(vkContext.logicalDevice, buffer->handle, NULL);
    vkFreeMemory(vkContext.logicalDevice, buffer->memory, NULL);
    free(buffer);
}

void VKK_DestroyBuffer(VKK_Buffer buffer) {
    if (!buffer)
        return;

    if (vkContext.pendingDeletionCount >= MAX_PENDING_DELETIONS) {
        LogError("VKK_DestroyBuffer: Max pending deletions exceeded");
        return;
    }

    PendingDeletion deletion = {
        .buffer = buffer,
        .framesUntilDeletion = MAX_FRAMES_IN_FLIGHT
    };

    vkContext.pendingDeletions[vkContext.pendingDeletionCount++] = deletion;
}
