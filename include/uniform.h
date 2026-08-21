#ifndef UNIFORM_H
#define UNIFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "buffer.h"

#include <vulkan/vulkan.h>

struct VKK_Uniform_T {
    VKK_Buffer buffer;
    VkShaderStageFlags stageFlags;
};

#ifdef __cplusplus
}
#endif

#endif