#ifndef VKK_SHARED_H
#define VKK_SHARED_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

#include "vkcontext.h"
#include "logger.h"

static bool logWarnings;

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkShaderStageFlags ConvertShaderStage(VKK_ShaderStage shaderStage);

#ifdef __cplusplus
}
#endif

#endif