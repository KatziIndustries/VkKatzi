#ifndef VKCONTEXT_H
#define VKCONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../include/vkkatzi.h"

#define MAX_PENDING_DELETIONS 4096
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_DRAW_CALLS 4096
#define MAX_UNIFORMS 32
#define MAX_TEXTURES 64

typedef struct {
    VkSwapchainKHR swapchainHandle;

    VkImageView* imageViews;
    VkImage* images;
    VkFramebuffer* framebuffers;
    uint32_t imageCount;

    VkExtent2D dimensions;

    VkFormat swapchainFormat;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR surfacePresentMode;
} VkSwapchain;

typedef enum {
    DELETION_BUFFER,
    DELETION_TEXTURE
} DeletionType;

typedef struct {
    DeletionType deletionType;
    uint32_t framesUntilDeletion;

    union {
        VKK_Buffer buffer;
        VKK_Texture texture;
    };
} PendingDeletion;

typedef struct {
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    uint32_t indexCount;
    VKK_Pipeline pipeline;

    VkBuffer instanceBuffer;
    uint32_t instanceCount;
} DrawCall;

typedef struct  {
    VkInstance instance;
    const char** layers;
    const char** extensions;
    uint32_t layersAmount;
    uint32_t extensionsAmount;

    VkSurfaceKHR surface;

    uint32_t windowWidth;
    uint32_t windowHeight;

    VkPhysicalDevice availablePhysicalDevices[16];
    uint32_t availableDeviceCount;
    VkPhysicalDevice physicalDevice;

    VkDevice logicalDevice;

    int32_t graphicsQueueFamilyIndex;
    int32_t presentQueueFamilyIndex;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchain swapchain;

    VkRenderPass renderPass;

    VkImage depthImage;
    VkDeviceMemory depthMemory;
    VkImageView depthImageView;
    VkFormat depthFormat; 

    DrawCall drawCalls[MAX_DRAW_CALLS];
    uint32_t drawCallIndex;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore* renderFinishedSemaphores;
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

    uint32_t currentFrame;

    PendingDeletion pendingDeletions[MAX_PENDING_DELETIONS];
    uint32_t pendingDeletionCount;

    bool pipelineFinalized;

    VKK_PushConstantRange pushConstantRange;
    void* pushConstantData;
    bool pushConstantDataSet;
} VkContext;

extern VkContext vkContext;

#ifdef __cplusplus
}
#endif

#endif