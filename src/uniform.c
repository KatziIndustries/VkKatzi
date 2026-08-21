#include "../include/uniform.h"
#include "../include/vkcontext.h"
#include "../include/logger.h"
#include "../include/shared.h"

VKK_Uniform VKK_CreateUniform(size_t size, VKK_ShaderStage stage) { 

    VKK_Uniform uniform = malloc(sizeof(struct VKK_Uniform_T));
    uniform->buffer = VKK_CreateBuffer(size, VKK_BUFFER_USAGE_UNIFORM);
    uniform->stageFlags = ConvertShaderStage(stage);

    if (!uniform->buffer) {
        free(uniform);
        return NULL;
    }

    return uniform;
}

void VKK_BindUniform(uint32_t binding, VKK_Uniform uniform) {

    if (!vkContext.descriptorSet) {
        LogError("VKK_InitRenderer has to be called before calling VKK_BindUniform");
        return;
    }
    
    const VkDescriptorBufferInfo bufferInfo = {
        .buffer = uniform->buffer->handle,
        .offset = 0,
        .range = uniform->buffer->size
    };

    const VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkContext.descriptorSet,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &bufferInfo,
    };

    vkUpdateDescriptorSets(vkContext.logicalDevice, 1, &descriptorWrite, 0, NULL);
}

void VKK_WriteUniform(VKK_Uniform uniform, const void* data, size_t size, size_t offset) {
    if (!uniform)
        return;

    VKK_WriteBuffer(uniform->buffer, data, size, offset);
}

void VKK_DestroyUniform(VKK_Uniform uniform) {
    VKK_DestroyBuffer(uniform->buffer);
}
