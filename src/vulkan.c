#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../include/shared.h"
#include "../include/vkkatzi.h"
#include "../include/logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

VkPresentModeKHR PREFERRED_PRESENT_MODE;
uint32_t DESIRED_IMAGE_COUNT = 0;

typedef struct {
    VkSurfaceFormatKHR* surfaceFormats;
    uint32_t formatCount;
    VkPresentModeKHR* surfacePresentModes;
    uint32_t presentModesCount;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
} VkSwapchainInfo;

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

#define MAX_PENDING_DELETIONS 4096

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

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_DRAW_CALLS 4096
#define MAX_UNIFORMS 32
#define MAX_TEXTURES 64

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

struct VKK_Buffer_T {
    VkBuffer handle;
    VkDeviceMemory memory;
    VkDeviceSize size;
    bool isMapped;
    void* mappedPtr;
};

struct VKK_Uniform_T {
    VKK_Buffer buffer;
    VkShaderStageFlags stageFlags;
};

struct VKK_Pipeline_T {
    VkPipeline handle;
    VkPipelineLayout layout;
};

struct VKK_Instance_T {
    VkInstance handle;
};

struct VKK_Surface_T {
    VkSurfaceKHR handle;
};

struct VKK_Texture_T {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkSampler sampler;
    uint32_t width, height;
};

static VkContext vkContext;

static bool CreateInstance(VKK_InstanceInfo* instanceInfo) {

    uint32_t apiVersion;
    vkEnumerateInstanceVersion(&apiVersion);

    uint32_t major = VK_API_VERSION_MAJOR(apiVersion);
    uint32_t minor = VK_API_VERSION_MINOR(apiVersion);
    uint32_t patch = VK_API_VERSION_PATCH(apiVersion);

    instanceInfo->versionMajor = major;
    instanceInfo->versionMinor = minor;
    instanceInfo->versionPatch = patch;

    const VkApplicationInfo applicationInfo = {
        .apiVersion = apiVersion,
        .applicationVersion = VK_MAKE_VERSION(4, 2, 0),
        .engineVersion = VK_MAKE_VERSION(4, 2, 0),
        .pApplicationName = "Katzi Game",
        .pEngineName = "Katzi Engine",
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO
    };

    const VkInstanceCreateInfo instanceCreateInfo = {
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = vkContext.layersAmount,
        .enabledExtensionCount = vkContext.extensionsAmount,
        .ppEnabledLayerNames = vkContext.layers,
        .ppEnabledExtensionNames = vkContext.extensions,
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    };

    if (vkCreateInstance(&instanceCreateInfo, NULL, &vkContext.instance) != VK_SUCCESS) {
        return false;
    }

    VKK_Instance instance = malloc(sizeof(struct VKK_Instance_T));
    instance->handle = vkContext.instance;

    instanceInfo->instance = instance;

    return true;
}

static bool FindQueueFamilies() {

    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vkContext.physicalDevice, &queueCount, NULL);

    VkQueueFamilyProperties properties[16];
    vkGetPhysicalDeviceQueueFamilyProperties(vkContext.physicalDevice, &queueCount, properties);

    int32_t graphicsQueueFamilyIndex = -1;
    int32_t presentQueueFamilyIndex = -1;

    for (uint32_t j = 0; j < queueCount; j++) {

        if (properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = j;

            VkBool32 supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(vkContext.physicalDevice, j, vkContext.surface, &supported);

            if (supported) {
                presentQueueFamilyIndex = j;

                vkContext.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
                vkContext.presentQueueFamilyIndex = presentQueueFamilyIndex;

                return true;
            }
        }
    }

    return false;
}

static bool CreateLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures) {

    VkDeviceQueueCreateInfo deviceQueueCreateInfo[2];

    float priority = 1.0f;
    uint32_t queuesAmount = 0;

    deviceQueueCreateInfo[queuesAmount++] = (VkDeviceQueueCreateInfo){
        .queueFamilyIndex = vkContext.graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &priority,
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
    };

    if (vkContext.graphicsQueueFamilyIndex != vkContext.presentQueueFamilyIndex) {

        deviceQueueCreateInfo[queuesAmount++] = (VkDeviceQueueCreateInfo){
            .queueFamilyIndex = vkContext.presentQueueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &priority,
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
        };
    }

    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const VkDeviceCreateInfo deviceCreateInfo = {
        .pQueueCreateInfos = deviceQueueCreateInfo,
        .queueCreateInfoCount = queuesAmount,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = deviceExtensions,
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pEnabledFeatures = &enabledFeatures,
    };

    if (vkCreateDevice(vkContext.physicalDevice, &deviceCreateInfo, NULL, &vkContext.logicalDevice) != VK_SUCCESS) {
        return false;
    }

    vkGetDeviceQueue(vkContext.logicalDevice, vkContext.graphicsQueueFamilyIndex, 0, &vkContext.graphicsQueue);
    vkGetDeviceQueue(vkContext.logicalDevice, vkContext.presentQueueFamilyIndex, 0, &vkContext.presentQueue);

    return true;
}

static void GetVkSwapchainInfo(VkSwapchainInfo* o_info) {

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkContext.physicalDevice, vkContext.surface, &o_info->surfaceCapabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkContext.physicalDevice, vkContext.surface, &o_info->formatCount, NULL);
    o_info->surfaceFormats = calloc(o_info->formatCount, sizeof(*o_info->surfaceFormats));

    vkGetPhysicalDeviceSurfaceFormatsKHR(vkContext.physicalDevice, vkContext.surface, &o_info->formatCount, o_info->surfaceFormats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkContext.physicalDevice, vkContext.surface, &o_info->presentModesCount, NULL);
    o_info->surfacePresentModes = calloc(o_info->presentModesCount, sizeof(*o_info->surfacePresentModes));

    vkGetPhysicalDeviceSurfacePresentModesKHR(vkContext.physicalDevice, vkContext.surface, &o_info->presentModesCount, o_info->surfacePresentModes);
}

VkSurfaceFormatKHR GetVkSwapchainFormat(VkSurfaceFormatKHR* formats, uint32_t formatsCount) {
    for (uint32_t i = 0; i < formatsCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return formats[i];
        }
    }

    return formats[0];
}

VkPresentModeKHR GetVkSwapchainPresentMode(VkPresentModeKHR* modes, uint32_t modesCount) {
    for (uint32_t i = 0; i < modesCount; i++) {
        if (modes[i] == PREFERRED_PRESENT_MODE) {
            return modes[i];
        }
    }

    LogWarn("Preferred present mode isn't supported, defaulting to FIFO");
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GetVkSwapchainExtent(const VkSurfaceCapabilitiesKHR* capabilities, uint32_t width, uint32_t height) {
    VkExtent2D extent = (VkExtent2D){
        .width = width,
        .height = height
    };

    extent.width = MAX(capabilities->minImageExtent.width, MIN(capabilities->maxImageExtent.width, extent.width));;
    extent.height = MAX(capabilities->minImageExtent.height, MIN(capabilities->maxImageExtent.height, extent.height));;

    return extent;
}

static char* PresentModeToString(VKK_PresentMode presentMode) {

    switch (presentMode) {
        case VKK_PRESENT_MODE_FIFO:
            return "FIFO";

        case VKK_PRESENT_MODE_FIFO_LATEST_READY:
            return "FIFO_Latest_Ready";
        
        case VKK_PRESENT_MODE_FIFO_RELAXED:
            return "FIFO_Relaxed";

        case VKK_PRESENT_MODE_IMMEDIATE:
            return "Immediate";

        case VKK_PRESENT_MODE_MAILBOX:
            return "Mailbox";

        case VKK_PRESENT_MODE_SHARED_CONTINOUS_REFRESH:
            return "Shared_Continuous_Refresh";

        case VKK_PRESENT_MODE_SHARED_DEMAND_REFRESH:
            return "Shared_Demand_Refresh";
    }
}

static bool CreateVkSwapchain(VkSwapchain* o_swapchain) {

    VkSwapchainInfo info;
    GetVkSwapchainInfo(&info);

    VkSurfaceFormatKHR format = GetVkSwapchainFormat(info.surfaceFormats, info.formatCount);
    VkPresentModeKHR presentMode = GetVkSwapchainPresentMode(info.surfacePresentModes, info.presentModesCount);

    VkExtent2D extent = GetVkSwapchainExtent(&info.surfaceCapabilities, vkContext.windowWidth, vkContext.windowHeight);

    free(info.surfaceFormats);
    free(info.surfacePresentModes);

    uint32_t imageCount;
    if (DESIRED_IMAGE_COUNT >= info.surfaceCapabilities.minImageCount) {
        imageCount = DESIRED_IMAGE_COUNT;
    } else {
        LogWarn("Desired image buffer size was too low; using min image count");
        imageCount = info.surfaceCapabilities.minImageCount;
    }

    if (info.surfaceCapabilities.maxImageCount > 0 && imageCount > info.surfaceCapabilities.maxImageCount) {
        LogWarn("Desired image count was too high; using max image count");
        imageCount = info.surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vkContext.surface,
        .minImageCount = imageCount,
        .imageFormat = format.format,
        .imageExtent = extent,
        .imageColorSpace = format.colorSpace,
        .presentMode = presentMode,
        .preTransform = info.surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .clipped = VK_TRUE,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };

    if (vkContext.graphicsQueueFamilyIndex != vkContext.presentQueueFamilyIndex) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainInfo.queueFamilyIndexCount = 2;

        uint32_t families[2] = {
            vkContext.graphicsQueueFamilyIndex,
            vkContext.presentQueueFamilyIndex
        };

        swapchainInfo.pQueueFamilyIndices = families;

    } else {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    if (vkCreateSwapchainKHR(vkContext.logicalDevice, &swapchainInfo, NULL, &o_swapchain->swapchainHandle) != VK_SUCCESS) {
        return false;
    }

    vkGetSwapchainImagesKHR(vkContext.logicalDevice, o_swapchain->swapchainHandle, &o_swapchain->imageCount, NULL);
    o_swapchain->images = calloc(o_swapchain->imageCount, sizeof(VkImage));
    vkGetSwapchainImagesKHR(vkContext.logicalDevice, o_swapchain->swapchainHandle, &o_swapchain->imageCount, o_swapchain->images);

    o_swapchain->imageViews = calloc(o_swapchain->imageCount, sizeof(VkImageView));

    o_swapchain->surfacePresentMode = presentMode;
    o_swapchain->swapchainFormat = format.format;
    o_swapchain->surfaceFormat = format;
    o_swapchain->dimensions = extent;

    for (uint32_t i = 0; i < o_swapchain->imageCount; i++) {
        const VkImageViewCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = o_swapchain->images[i],
            .format = o_swapchain->swapchainFormat,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        if (vkCreateImageView(vkContext.logicalDevice, &info, NULL, &o_swapchain->imageViews[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

static bool CreateRenderPass() {

    const VkAttachmentDescription attachments[] = {

        {
            .format = vkContext.swapchain.swapchainFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        },

        {
            .format = vkContext.depthFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        }
    };



    const VkAttachmentReference colorAttachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkAttachmentReference depthAttachmentReference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference,
        .pDepthStencilAttachment = &depthAttachmentReference
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    const VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if (vkCreateRenderPass(vkContext.logicalDevice, &renderPassCreateInfo, NULL, &vkContext.renderPass) != VK_SUCCESS) {
        return false;
    }

    return true;
}

static bool CreateFrameBuffers(VkSwapchain* swapchain) {

    swapchain->framebuffers = calloc(swapchain->imageCount, sizeof(VkFramebuffer));

    for (uint32_t i = 0; i < swapchain->imageCount; i++) {
        
        const VkImageView attachments[] = {
            swapchain->imageViews[i],
            vkContext.depthImageView
        };

        const VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vkContext.renderPass,
            .attachmentCount = 2,
            .pAttachments = attachments,
            .width = swapchain->dimensions.width,
            .height = swapchain->dimensions.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(vkContext.logicalDevice, &framebufferInfo, NULL, &swapchain->framebuffers[i]) != VK_SUCCESS) {
            return false;
        }

    }

    return true;
}

static bool CreateCommandPool() {

    const VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vkContext.graphicsQueueFamilyIndex
    };

    if (vkCreateCommandPool(vkContext.logicalDevice, &poolInfo, NULL, &vkContext.commandPool) != VK_SUCCESS) {
        return false;
    }

    return true;
}

static bool CreateCommandBuffers() {
    vkContext.commandBuffers = calloc(vkContext.swapchain.imageCount, sizeof(VkCommandBuffer));

    const VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkContext.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = vkContext.swapchain.imageCount   
    };

    if (vkAllocateCommandBuffers(vkContext.logicalDevice, &allocateInfo, vkContext.commandBuffers) != VK_SUCCESS) {
        return false;
    }

    return true;
}

static char* ReadFile(const char* path, size_t* o_size) {
    FILE* file = fopen(path, "rb");

    if (!file) {
        fprintf(stderr, "Failed to open file %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *o_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(*o_size);
    fread(buffer, 1, *o_size, file);
    fclose(file);

    return buffer;
}

static VkShaderModule CreateShaderModule(const char* path) {
    size_t codeSize;
    char* code = ReadFile(path, &codeSize);

    if (!code)
        return VK_NULL_HANDLE;

    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = codeSize,
        .pCode = (const uint32_t*)code
    };

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(vkContext.logicalDevice, &shaderModuleCreateInfo, NULL, &shaderModule);
    free(code);

    if (result != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

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

static void QueueBufferDeletion(VKK_Buffer buffer) {

    PendingDeletion deletion = {
        .deletionType = DELETION_BUFFER,
        .framesUntilDeletion = MAX_FRAMES_IN_FLIGHT,
        .buffer = buffer,
    };

    vkContext.pendingDeletions[vkContext.pendingDeletionCount++] = deletion;
}

static void QueueTextureDeletion(VKK_Texture texture) {

    PendingDeletion deletion = {
        .deletionType = DELETION_TEXTURE,
        .framesUntilDeletion = MAX_FRAMES_IN_FLIGHT,
        .texture = texture
    };

    vkContext.pendingDeletions[vkContext.pendingDeletionCount++] = deletion;
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

void VKK_DestroyUniform(VKK_Uniform uniform) {
    VKK_DestroyBuffer(uniform->buffer);
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

static VkShaderStageFlags ConvertShaderStage(VKK_ShaderStage shaderStage) {
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

static VkFormat ConvertVertexFormat(VKK_VertexFormat format) {

    switch (format) {
        case VKK_VERTEX_FORMAT_FLOAT:
            return VK_FORMAT_R32_SFLOAT;

        case VKK_VERTEX_FORMAT_FLOAT2:
            return VK_FORMAT_R32G32_SFLOAT;

        case VKK_VERTEX_FORMAT_FLOAT3:
            return VK_FORMAT_R32G32B32_SFLOAT;

        case VKK_VERTEX_FORMAT_FLOAT4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;

        case VKK_VERTEX_FORMAT_INT:
            return VK_FORMAT_R32_SINT;

        case VKK_VERTEX_FORMAT_INT2:
            return VK_FORMAT_R32G32_SINT;

        case VKK_VERTEX_FORMAT_INT3:
            return VK_FORMAT_R32G32B32_SINT;

        case VKK_VERTEX_FORMAT_INT4:
            return VK_FORMAT_R32G32B32A32_SINT;
    }

    LogError("Vertex format couldn't be converted (this is not supposed to happen wtf did you do?)");
    return VK_FORMAT_R32_SFLOAT;
}

VKK_Pipeline VKK_CreatePipeline(VKK_PipelineDescription desc) {

    VkShaderModule vertModule = CreateShaderModule(desc.vertexShaderPath);
    VkShaderModule fragModule = CreateShaderModule(desc.fragmentShaderPath);

    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        return false;
    }

    const VkPipelineShaderStageCreateInfo shaderStages[] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertModule,
            .pName = "main"
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragModule,
            .pName = "main"
        }
    };

    VkVertexInputBindingDescription bindingDescriptions[2];
    uint32_t bindingCount = 1;

    bindingDescriptions[0] = (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = desc.vertexStride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX   
    };

    if (desc.instanceStride > 0) {
        bindingDescriptions[1] = (VkVertexInputBindingDescription){
            .binding = 1,
            .stride = desc.instanceStride,
            .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE   
        };

        bindingCount = 2;
    }

    VkVertexInputAttributeDescription attributeDescriptions[32];
    uint32_t attributeIndex = 0;

    for (uint32_t i = 0; i < desc.attributeCount; i++) {
        attributeDescriptions[attributeIndex++] = (VkVertexInputAttributeDescription){
            .binding = 0,
            .location = desc.attributes[i].location,
            .format = ConvertVertexFormat(desc.attributes[i].format),
            .offset = desc.attributes[i].offset
        };
    }

    for (uint32_t i = 0; i < desc.instanceAttributesCount; i++) {
        attributeDescriptions[attributeIndex++] = (VkVertexInputAttributeDescription){
            .binding = 1,
            .location = desc.instanceAttributes[i].location,
            .format = ConvertVertexFormat(desc.instanceAttributes[i].format),
            .offset = desc.instanceAttributes[i].offset
        };
    }

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = bindingCount,
        .pVertexBindingDescriptions = bindingDescriptions,
        .vertexAttributeDescriptionCount = attributeIndex,
        .pVertexAttributeDescriptions = attributeDescriptions
    };

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
    };

    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    const VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = desc.rasterizer.polygonMode,
        .cullMode = desc.rasterizer.cullMode,
        .frontFace = desc.rasterizer.frontFace,
        .lineWidth = desc.rasterizer.lineWidth
    };


    const VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    const VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    const VkPushConstantRange pushConstantRange = {
        .stageFlags = ConvertShaderStage(vkContext.pushConstantRange.shaderStage),
        .offset = vkContext.pushConstantRange.offset,
        .size = vkContext.pushConstantRange.size,
    };

    const VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vkContext.descriptorSetLayout,
        .pushConstantRangeCount = vkContext.pushConstantRange.size > 0 ? 1 : 0,
        .pPushConstantRanges = vkContext.pushConstantRange.size > 0 ? &pushConstantRange : NULL
    };

    VKK_Pipeline pipeline = malloc(sizeof(struct VKK_Pipeline_T));

    if (vkCreatePipelineLayout(vkContext.logicalDevice, &layoutInfo, NULL, &pipeline->layout) != VK_SUCCESS) {
        free(pipeline);
        return NULL;
    }

    const VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = desc.enableDepthTesting,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = desc.depthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = pipeline->layout,
        .renderPass = vkContext.renderPass,
        .subpass = 0,
        .pDynamicState = &dynamicState
    };

    if (vkCreateGraphicsPipelines(vkContext.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline->handle) != VK_SUCCESS) {
        vkDestroyPipelineLayout(vkContext.logicalDevice, pipeline->layout, NULL);
        free(pipeline);
        return NULL;
    }

    vkDestroyShaderModule(vkContext.logicalDevice, vertModule, NULL);
    vkDestroyShaderModule(vkContext.logicalDevice, fragModule, NULL);

    return pipeline;
}

void VKK_DestroyPipeline(VKK_Pipeline pipeline) {

    if (!pipeline)
        return;

    vkDeviceWaitIdle(vkContext.logicalDevice);
    vkDestroyPipeline(vkContext.logicalDevice, pipeline->handle, NULL);
    vkDestroyPipelineLayout(vkContext.logicalDevice, pipeline->layout, NULL);
    free(pipeline);
}

static bool CreateSyncObjects() {
    const VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    const VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(vkContext.logicalDevice, &semaphoreInfo, NULL, &vkContext.imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkContext.logicalDevice, &fenceInfo, NULL, &vkContext.inFlightFences[i]) != VK_SUCCESS) {
                return false;
            }
    }

    vkContext.renderFinishedSemaphores = calloc(vkContext.swapchain.imageCount, sizeof(VkSemaphore));

    for (uint32_t i = 0; i < vkContext.swapchain.imageCount; i++) {
        if (vkCreateSemaphore(vkContext.logicalDevice, &semaphoreInfo, NULL, &vkContext.renderFinishedSemaphores[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

static bool RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VKK_Color clear) {

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        return false;
    }

    const VkClearValue clearValues[2] = {
        {
            .color = (VkClearColorValue){
            .float32 = {clear.r, clear.g, clear.b, clear.a}
            },   


        },

        {
            .depthStencil = {
                .depth = 1.0,
                .stencil = 0
            }
        }
    };

    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vkContext.renderPass,
        .framebuffer = vkContext.swapchain.framebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = vkContext.swapchain.dimensions
        },
        .clearValueCount = 2,
        .pClearValues = clearValues
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)vkContext.swapchain.dimensions.width,
        .height = (float)vkContext.swapchain.dimensions.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = vkContext.swapchain.dimensions
    };
    
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    if (vkContext.drawCallIndex > 0) {
        
        if (vkContext.pushConstantDataSet) {
            vkCmdPushConstants(commandBuffer, vkContext.drawCalls[0].pipeline->layout, ConvertShaderStage(vkContext.pushConstantRange.shaderStage), vkContext.pushConstantRange.offset, vkContext.pushConstantRange.size, vkContext.pushConstantData);
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.drawCalls[0].pipeline->layout, 0, 1, &vkContext.descriptorSet, 0, NULL);

        for (uint32_t i = 0; i < vkContext.drawCallIndex; i++) {
            
            DrawCall* drawCall = &vkContext.drawCalls[i];

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCall->pipeline->handle);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &drawCall->vertexBuffer, &offset);


            if (drawCall->instanceBuffer != VK_NULL_HANDLE) {
                vkCmdBindVertexBuffers(commandBuffer, 1, 1, &drawCall->instanceBuffer, &offset);
            }

            vkCmdBindIndexBuffer(commandBuffer, drawCall->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCall->pipeline->layout, 0, 1, &vkContext.descriptorSet, 0, NULL);
        
            vkCmdDrawIndexed(commandBuffer, drawCall->indexCount, drawCall->instanceCount, 0, 0, 0);
        }

    }

    vkContext.drawCallIndex = 0;

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void VKK_Draw(VKK_Pipeline pipeline, VKK_Buffer vertexBuffer, VKK_Buffer indexBuffer, uint32_t indexCount) {

    if (vkContext.drawCallIndex >= MAX_DRAW_CALLS) {
        fprintf(stderr, "[VKK][ERROR]: Max draw calls (%d) in frame reached, dropping draw call\n", MAX_DRAW_CALLS);
        return;
    }

    DrawCall drawCall = {
        .vertexBuffer = vertexBuffer->handle,
        .indexBuffer = indexBuffer->handle,
        .indexCount = indexCount,
        .pipeline = pipeline,
        .instanceBuffer = VK_NULL_HANDLE,
        .instanceCount = 1
    };

    vkContext.drawCalls[vkContext.drawCallIndex++] = drawCall;
}

void VKK_DrawInstanced(VKK_Pipeline pipeline, VKK_Buffer vertexBuffer, VKK_Buffer indexBuffer, uint32_t indexCount, VKK_Buffer instanceBuffer, uint32_t instanceCount) {

    if (vkContext.drawCallIndex >= MAX_DRAW_CALLS) {
        fprintf(stderr, "[VKK][ERROR]: Max draw calls (%d) in frame reached, dropping draw call\n", MAX_DRAW_CALLS);
        return;
    }

    DrawCall drawCall = {
        .vertexBuffer = vertexBuffer->handle,
        .indexBuffer = indexBuffer->handle,
        .indexCount = indexCount,
        .pipeline = pipeline,
        .instanceBuffer = instanceBuffer ? instanceBuffer->handle : VK_NULL_HANDLE,
        .instanceCount = instanceCount
    };

    vkContext.drawCalls[vkContext.drawCallIndex++] = drawCall;
}

static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

VKK_Result VKK_CreateDescriptorSetLayout(VKK_DescriptorSetLayoutBinding* bindings, uint32_t bindingsCount) {

    VkDescriptorSetLayoutBinding vkBindings[bindingsCount];

    for (uint32_t i = 0; i < bindingsCount; i++) {
        vkBindings[i] = (VkDescriptorSetLayoutBinding){
            .binding = bindings[i].binding,
            .descriptorType = (VkDescriptorType)bindings[i].descriptorType,
            .descriptorCount = 1,
            .stageFlags = ConvertShaderStage(bindings[i].shaderStage),
        };
    }

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = bindingsCount,
        .pBindings = vkBindings,
    };

    if (vkCreateDescriptorSetLayout(vkContext.logicalDevice, &layoutInfo, NULL, &vkContext.descriptorSetLayout) != VK_SUCCESS) {
        return VKK_ERROR_DESCRIPTOR_SET_LAYOUT_CREATION_FAILED;
    }

    return VKK_SUCCESS;
}

static bool CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImage* o_image, VkDeviceMemory* o_memory) {

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

static bool CreateDepthResources() {
    vkContext.depthFormat = VK_FORMAT_D32_SFLOAT;

    VkExtent2D extent = vkContext.swapchain.dimensions;

    if (!CreateImage(extent.width, extent.height, vkContext.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &vkContext.depthImage, &vkContext.depthMemory)) {
        return false;
    }

    VkImageViewCreateInfo imageViewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = vkContext.depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vkContext.depthFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1   
        },
    };

    if (vkCreateImageView(vkContext.logicalDevice, &imageViewInfo, NULL, &vkContext.depthImageView) != VK_SUCCESS) {
        return false;
    }

    return true;
}
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

static void CleanupSwapchain() { 
    for (uint32_t i = 0; i < vkContext.swapchain.imageCount; i++) {
        vkDestroyFramebuffer(vkContext.logicalDevice, vkContext.swapchain.framebuffers[i], NULL);
        vkDestroyImageView(vkContext.logicalDevice, vkContext.swapchain.imageViews[i], NULL);
    }

    free(vkContext.swapchain.framebuffers);
    free(vkContext.swapchain.imageViews);
    free(vkContext.swapchain.images);

    vkDestroySwapchainKHR(vkContext.logicalDevice, vkContext.swapchain.swapchainHandle, NULL);
}

static bool RecreateSwapchain() {

    vkDeviceWaitIdle(vkContext.logicalDevice);

    CleanupSwapchain();

    if (vkContext.depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(vkContext.logicalDevice, vkContext.depthImageView, NULL);
    }

    if (vkContext.depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(vkContext.logicalDevice, vkContext.depthImage, NULL);
    }

    if (vkContext.depthMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vkContext.logicalDevice, vkContext.depthMemory, NULL);
    }



    if (!CreateVkSwapchain(&vkContext.swapchain)) {
        return false;
    }

    if (!CreateDepthResources()) {
        return false;
    }

    if (!CreateFrameBuffers(&vkContext.swapchain)) {
        return false;
    }

    return true;
}

void VKK_SetFramebufferSize(uint32_t width, uint32_t height) {
    vkContext.windowWidth = width;
    vkContext.windowHeight = height;
    RecreateSwapchain();
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

static bool CreateDescriptorPoolAndSet(uint32_t maxSets) {

    const VkDescriptorPoolSize poolSizes[] = {
        { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = MAX_UNIFORMS },
        { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = MAX_TEXTURES },
    };

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 2,
        .pPoolSizes = poolSizes,
        .maxSets = maxSets,
    };

    if (vkCreateDescriptorPool(vkContext.logicalDevice, &poolInfo, NULL, &vkContext.descriptorPool) != VK_SUCCESS) {
        return false;
    }

    const VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkContext.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vkContext.descriptorSetLayout
    };

    if (vkAllocateDescriptorSets(vkContext.logicalDevice, &allocateInfo, &vkContext.descriptorSet) != VK_SUCCESS) {
        return false;
    }

    return true;
}

static void DestroyBuffer(VKK_Buffer buffer) {
    vkDestroyBuffer(vkContext.logicalDevice, buffer->handle, NULL);
    vkFreeMemory(vkContext.logicalDevice, buffer->memory, NULL);
    free(buffer);
}

static void DestroyTexture(VKK_Texture texture) {

    if (!texture)
        return;

    vkDestroySampler(vkContext.logicalDevice, texture->sampler, NULL);
    vkDestroyImageView(vkContext.logicalDevice, texture->imageView, NULL);
    vkDestroyImage(vkContext.logicalDevice, texture->image, NULL);
    vkFreeMemory(vkContext.logicalDevice, texture->memory, NULL);
    free(texture);
}

static void HandleDeletion(PendingDeletion* deletion) {

    switch (deletion->deletionType) {

        case DELETION_BUFFER:
            DestroyBuffer(deletion->buffer);
            break;

        case DELETION_TEXTURE:
            DestroyTexture(deletion->texture);
            break;
    }

}

static void ProcessPendingDeletions() {

    for (uint32_t i = 0; i < vkContext.pendingDeletionCount;) {
        vkContext.pendingDeletions[i].framesUntilDeletion--;

        if (vkContext.pendingDeletions[i].framesUntilDeletion <= 0) {

            HandleDeletion(&vkContext.pendingDeletions[i]);
            vkContext.pendingDeletions[i] = vkContext.pendingDeletions[--vkContext.pendingDeletionCount];

        } else {
            i++;
        }
    }
}

static VkPhysicalDeviceProperties FillPhysicalDeviceProperties(VkPhysicalDevice device, VKK_PhysicalDeviceProperties* o_deviceInfo) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    strncpy(o_deviceInfo->deviceName, properties.deviceName, VKK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    o_deviceInfo->apiVersion = properties.apiVersion;
    o_deviceInfo->driverVersion = properties.driverVersion;
    o_deviceInfo->vendorID = properties.vendorID;
    o_deviceInfo->deviceID = properties.deviceID;
    o_deviceInfo->deviceType = properties.deviceType;

    // I don't know if this is being filled in correctly and I will not check if it is
    o_deviceInfo->limits.maxImageDimension1D = properties.limits.maxImageDimension1D;
    o_deviceInfo->limits.maxImageDimension2D = properties.limits.maxImageDimension2D;
    o_deviceInfo->limits.maxImageDimension3D = properties.limits.maxImageDimension3D;
    o_deviceInfo->limits.maxImageDimensionCube = properties.limits.maxImageDimensionCube;
    o_deviceInfo->limits.maxImageArrayLayers = properties.limits.maxImageArrayLayers;
    o_deviceInfo->limits.maxTexelBufferElements = properties.limits.maxTexelBufferElements;
    o_deviceInfo->limits.maxUniformBufferRange = properties.limits.maxUniformBufferRange;
    o_deviceInfo->limits.maxStorageBufferRange = properties.limits.maxStorageBufferRange;
    o_deviceInfo->limits.maxPushConstantsSize = properties.limits.maxPushConstantsSize;
    o_deviceInfo->limits.maxMemoryAllocationCount = properties.limits.maxMemoryAllocationCount;
    o_deviceInfo->limits.maxSamplerAllocationCount = properties.limits.maxSamplerAllocationCount;
    o_deviceInfo->limits.bufferImageGranularity = properties.limits.bufferImageGranularity;
    o_deviceInfo->limits.sparseAddressSpaceSize = properties.limits.sparseAddressSpaceSize;
    o_deviceInfo->limits.maxBoundDescriptorSets = properties.limits.maxBoundDescriptorSets;
    o_deviceInfo->limits.maxPerStageDescriptorSamplers = properties.limits.maxPerStageDescriptorSamplers;
    o_deviceInfo->limits.maxPerStageDescriptorUniformBuffers = properties.limits.maxPerStageDescriptorUniformBuffers;
    o_deviceInfo->limits.maxPerStageDescriptorStorageBuffers = properties.limits.maxPerStageDescriptorStorageBuffers;
    o_deviceInfo->limits.maxPerStageDescriptorSampledImages = properties.limits.maxPerStageDescriptorSampledImages;
    o_deviceInfo->limits.maxPerStageDescriptorStorageImages = properties.limits.maxPerStageDescriptorStorageImages;
    o_deviceInfo->limits.maxPerStageDescriptorInputAttachments = properties.limits.maxPerStageDescriptorInputAttachments;
    o_deviceInfo->limits.maxPerStageResources = properties.limits.maxPerStageResources;
    o_deviceInfo->limits.maxDescriptorSetSamplers = properties.limits.maxDescriptorSetSamplers;
    o_deviceInfo->limits.maxDescriptorSetUniformBuffers = properties.limits.maxDescriptorSetUniformBuffers;
    o_deviceInfo->limits.maxDescriptorSetUniformBuffersDynamic = properties.limits.maxDescriptorSetUniformBuffersDynamic;
    o_deviceInfo->limits.maxDescriptorSetStorageBuffers = properties.limits.maxDescriptorSetStorageBuffers;
    o_deviceInfo->limits.maxDescriptorSetStorageBuffersDynamic = properties.limits.maxDescriptorSetStorageBuffersDynamic;
    o_deviceInfo->limits.maxDescriptorSetSampledImages = properties.limits.maxDescriptorSetSampledImages;
    o_deviceInfo->limits.maxDescriptorSetStorageImages = properties.limits.maxDescriptorSetStorageImages;
    o_deviceInfo->limits.maxDescriptorSetInputAttachments = properties.limits.maxDescriptorSetInputAttachments;
    o_deviceInfo->limits.maxVertexInputAttributes = properties.limits.maxVertexInputAttributes;
    o_deviceInfo->limits.maxVertexInputBindings = properties.limits.maxVertexInputBindings;
    o_deviceInfo->limits.maxVertexInputAttributeOffset = properties.limits.maxVertexInputAttributeOffset;
    o_deviceInfo->limits.maxVertexInputBindingStride = properties.limits.maxVertexInputBindingStride;
    o_deviceInfo->limits.maxVertexOutputComponents = properties.limits.maxVertexOutputComponents;
    o_deviceInfo->limits.maxTessellationGenerationLevel = properties.limits.maxTessellationGenerationLevel;
    o_deviceInfo->limits.maxTessellationPatchSize = properties.limits.maxTessellationPatchSize;
    o_deviceInfo->limits.maxTessellationControlPerVertexInputComponents = properties.limits.maxTessellationControlPerVertexInputComponents;
    o_deviceInfo->limits.maxTessellationControlPerVertexOutputComponents = properties.limits.maxTessellationControlPerVertexOutputComponents;
    o_deviceInfo->limits.maxTessellationControlPerPatchOutputComponents = properties.limits.maxTessellationControlPerPatchOutputComponents;
    o_deviceInfo->limits.maxTessellationControlTotalOutputComponents = properties.limits.maxTessellationControlTotalOutputComponents;
    o_deviceInfo->limits.maxTessellationEvaluationInputComponents = properties.limits.maxTessellationEvaluationInputComponents;
    o_deviceInfo->limits.maxTessellationEvaluationOutputComponents = properties.limits.maxTessellationEvaluationOutputComponents;
    o_deviceInfo->limits.maxGeometryShaderInvocations = properties.limits.maxGeometryShaderInvocations;
    o_deviceInfo->limits.maxGeometryInputComponents = properties.limits.maxGeometryInputComponents;
    o_deviceInfo->limits.maxGeometryOutputComponents = properties.limits.maxGeometryOutputComponents;
    o_deviceInfo->limits.maxGeometryOutputVertices = properties.limits.maxGeometryOutputVertices;
    o_deviceInfo->limits.maxGeometryTotalOutputComponents = properties.limits.maxGeometryTotalOutputComponents;
    o_deviceInfo->limits.maxFragmentInputComponents = properties.limits.maxFragmentInputComponents;
    o_deviceInfo->limits.maxFragmentOutputAttachments = properties.limits.maxFragmentOutputAttachments;
    o_deviceInfo->limits.maxFragmentDualSrcAttachments = properties.limits.maxFragmentDualSrcAttachments;
    o_deviceInfo->limits.maxFragmentCombinedOutputResources = properties.limits.maxFragmentCombinedOutputResources;
    o_deviceInfo->limits.maxComputeSharedMemorySize = properties.limits.maxComputeSharedMemorySize;
    //Arrays can't be filled in ig. Not my problem
    //o_deviceInfo->limits.maxComputeWorkGroupCount = properties.limits.maxComputeWorkGroupCount;
    o_deviceInfo->limits.maxComputeWorkGroupInvocations = properties.limits.maxComputeWorkGroupInvocations;
    //o_deviceInfo->limits.maxComputeWorkGroupSize = properties.limits.maxComputeWorkGroupSize;
    o_deviceInfo->limits.subPixelPrecisionBits = properties.limits.subPixelPrecisionBits;
    o_deviceInfo->limits.subTexelPrecisionBits = properties.limits.subTexelPrecisionBits;
    o_deviceInfo->limits.mipmapPrecisionBits = properties.limits.mipmapPrecisionBits;
    o_deviceInfo->limits.maxDrawIndexedIndexValue = properties.limits.maxDrawIndexedIndexValue;
    o_deviceInfo->limits.maxDrawIndirectCount = properties.limits.maxDrawIndirectCount;
    o_deviceInfo->limits.maxSamplerLodBias = properties.limits.maxSamplerLodBias;
    o_deviceInfo->limits.maxSamplerAnisotropy = properties.limits.maxSamplerAnisotropy;
    o_deviceInfo->limits.maxViewports = properties.limits.maxViewports;
    //o_deviceInfo->limits.maxViewportDimensions = properties.limits.maxViewportDimensions;
    //o_deviceInfo->limits.viewportBoundsRange = properties.limits.viewportBoundsRange;
    o_deviceInfo->limits.viewportSubPixelBits = properties.limits.viewportSubPixelBits;
    o_deviceInfo->limits.minMemoryMapAlignment = properties.limits.minMemoryMapAlignment;
    o_deviceInfo->limits.minTexelBufferOffsetAlignment = properties.limits.minTexelBufferOffsetAlignment;
    o_deviceInfo->limits.minUniformBufferOffsetAlignment = properties.limits.minUniformBufferOffsetAlignment;
    o_deviceInfo->limits.minStorageBufferOffsetAlignment = properties.limits.minStorageBufferOffsetAlignment;
    o_deviceInfo->limits.minTexelOffset = properties.limits.minTexelOffset;
    o_deviceInfo->limits.maxTexelOffset = properties.limits.maxTexelOffset;
    o_deviceInfo->limits.minTexelGatherOffset = properties.limits.minTexelGatherOffset;
    o_deviceInfo->limits.maxTexelGatherOffset = properties.limits.maxTexelGatherOffset;
    o_deviceInfo->limits.minInterpolationOffset = properties.limits.minInterpolationOffset;
    o_deviceInfo->limits.maxInterpolationOffset = properties.limits.maxInterpolationOffset;
    o_deviceInfo->limits.subPixelInterpolationOffsetBits = properties.limits.subPixelInterpolationOffsetBits;
    o_deviceInfo->limits.maxFramebufferWidth = properties.limits.maxFramebufferWidth;
    o_deviceInfo->limits.maxFramebufferHeight = properties.limits.maxFramebufferHeight;
    o_deviceInfo->limits.maxFramebufferLayers = properties.limits.maxFramebufferLayers;
    o_deviceInfo->limits.framebufferColorSampleCounts = properties.limits.framebufferColorSampleCounts;
    o_deviceInfo->limits.framebufferDepthSampleCounts = properties.limits.framebufferDepthSampleCounts;
    o_deviceInfo->limits.framebufferStencilSampleCounts = properties.limits.framebufferStencilSampleCounts;
    o_deviceInfo->limits.framebufferNoAttachmentsSampleCounts = properties.limits.framebufferNoAttachmentsSampleCounts;
    o_deviceInfo->limits.maxColorAttachments = properties.limits.maxColorAttachments;
    o_deviceInfo->limits.sampledImageColorSampleCounts = properties.limits.sampledImageColorSampleCounts;
    o_deviceInfo->limits.sampledImageIntegerSampleCounts = properties.limits.sampledImageIntegerSampleCounts;
    o_deviceInfo->limits.sampledImageDepthSampleCounts = properties.limits.sampledImageDepthSampleCounts;
    o_deviceInfo->limits.sampledImageStencilSampleCounts = properties.limits.sampledImageStencilSampleCounts;
    o_deviceInfo->limits.storageImageSampleCounts = properties.limits.storageImageSampleCounts;
    o_deviceInfo->limits.maxSampleMaskWords = properties.limits.maxSampleMaskWords;
    o_deviceInfo->limits.timestampComputeAndGraphics = properties.limits.timestampComputeAndGraphics;
    o_deviceInfo->limits.timestampPeriod = properties.limits.timestampPeriod;
    o_deviceInfo->limits.maxClipDistances = properties.limits.maxClipDistances;
    o_deviceInfo->limits.maxCullDistances = properties.limits.maxCullDistances;
    o_deviceInfo->limits.maxCombinedClipAndCullDistances = properties.limits.maxCombinedClipAndCullDistances;
    o_deviceInfo->limits.discreteQueuePriorities = properties.limits.discreteQueuePriorities;
    //o_deviceInfo->limits.pointSizeRange = properties.limits.pointSizeRange;
    //o_deviceInfo->limits.lineWidthRange = properties.limits.lineWidthRange;
    o_deviceInfo->limits.pointSizeGranularity = properties.limits.pointSizeGranularity;
    o_deviceInfo->limits.lineWidthGranularity = properties.limits.lineWidthGranularity;
    o_deviceInfo->limits.strictLines = properties.limits.strictLines;
    o_deviceInfo->limits.standardSampleLocations = properties.limits.standardSampleLocations;
    o_deviceInfo->limits.optimalBufferCopyOffsetAlignment = properties.limits.optimalBufferCopyOffsetAlignment;
    o_deviceInfo->limits.optimalBufferCopyRowPitchAlignment = properties.limits.optimalBufferCopyRowPitchAlignment;
    o_deviceInfo->limits.nonCoherentAtomSize = properties.limits.nonCoherentAtomSize;

    o_deviceInfo->sparseProperties.residencyStandard2DBlockShape = properties.sparseProperties.residencyStandard2DBlockShape;
    o_deviceInfo->sparseProperties.residencyStandard2DMultisampleBlockShape = properties.sparseProperties.residencyStandard2DMultisampleBlockShape;
    o_deviceInfo->sparseProperties.residencyStandard3DBlockShape = properties.sparseProperties.residencyStandard3DBlockShape;
    o_deviceInfo->sparseProperties.residencyAlignedMipSize = properties.sparseProperties.residencyAlignedMipSize;
    o_deviceInfo->sparseProperties.residencyNonResidentStrict = properties.sparseProperties.residencyNonResidentStrict;

    return properties;
}

static VkPhysicalDeviceFeatures FillPhysicalDeviceFeatures(VkPhysicalDevice device, VKK_PhysicalDeviceFeatures* o_deviceFeatures) {

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    o_deviceFeatures->robustBufferAccess = features.robustBufferAccess;
    o_deviceFeatures->fullDrawIndexUint32 = features.fullDrawIndexUint32;
    o_deviceFeatures->imageCubeArray = features.imageCubeArray;
    o_deviceFeatures->independentBlend = features.independentBlend;
    o_deviceFeatures->geometryShader = features.geometryShader;
    o_deviceFeatures->tessellationShader = features.tessellationShader;
    o_deviceFeatures->sampleRateShading = features.sampleRateShading;
    o_deviceFeatures->dualSrcBlend = features.dualSrcBlend;
    o_deviceFeatures->logicOp = features.logicOp;
    o_deviceFeatures->multiDrawIndirect = features.multiDrawIndirect;
    o_deviceFeatures->drawIndirectFirstInstance = features.drawIndirectFirstInstance;
    o_deviceFeatures->depthClamp = features.depthClamp;
    o_deviceFeatures->depthBiasClamp = features.depthBiasClamp;
    o_deviceFeatures->fillModeNonSolid = features.fillModeNonSolid;
    o_deviceFeatures->depthBounds = features.depthBounds;
    o_deviceFeatures->wideLines = features.wideLines;
    o_deviceFeatures->largePoints = features.largePoints;
    o_deviceFeatures->alphaToOne = features.alphaToOne;
    o_deviceFeatures->multiViewport = features.multiViewport;
    o_deviceFeatures->samplerAnisotropy = features.samplerAnisotropy;
    o_deviceFeatures->textureCompressionETC2 = features.textureCompressionETC2;
    o_deviceFeatures->textureCompressionASTC_LDR = features.textureCompressionASTC_LDR;
    o_deviceFeatures->textureCompressionBC = features.textureCompressionBC;
    o_deviceFeatures->occlusionQueryPrecise = features.occlusionQueryPrecise;
    o_deviceFeatures->pipelineStatisticsQuery = features.pipelineStatisticsQuery;
    o_deviceFeatures->vertexPipelineStoresAndAtomics = features.vertexPipelineStoresAndAtomics;
    o_deviceFeatures->fragmentStoresAndAtomics = features.fragmentStoresAndAtomics;
    o_deviceFeatures->shaderTessellationAndGeometryPointSize = features.shaderTessellationAndGeometryPointSize;
    o_deviceFeatures->shaderImageGatherExtended = features.shaderImageGatherExtended;
    o_deviceFeatures->shaderStorageImageExtendedFormats = features.shaderStorageImageExtendedFormats;
    o_deviceFeatures->shaderStorageImageMultisample = features.shaderStorageImageMultisample;
    o_deviceFeatures->shaderStorageImageReadWithoutFormat = features.shaderStorageImageReadWithoutFormat;
    o_deviceFeatures->shaderStorageImageWriteWithoutFormat = features.shaderStorageImageWriteWithoutFormat;
    o_deviceFeatures->shaderUniformBufferArrayDynamicIndexing = features.shaderUniformBufferArrayDynamicIndexing;
    o_deviceFeatures->shaderSampledImageArrayDynamicIndexing = features.shaderSampledImageArrayDynamicIndexing;
    o_deviceFeatures->shaderStorageBufferArrayDynamicIndexing = features.shaderStorageBufferArrayDynamicIndexing;
    o_deviceFeatures->shaderStorageImageArrayDynamicIndexing = features.shaderStorageImageArrayDynamicIndexing;
    o_deviceFeatures->shaderClipDistance = features.shaderClipDistance;
    o_deviceFeatures->shaderCullDistance = features.shaderCullDistance;
    o_deviceFeatures->shaderFloat64 = features.shaderFloat64;
    o_deviceFeatures->shaderInt64 = features.shaderInt64;
    o_deviceFeatures->shaderInt16 = features.shaderInt16;
    o_deviceFeatures->shaderResourceResidency = features.shaderResourceResidency;
    o_deviceFeatures->shaderResourceMinLod = features.shaderResourceMinLod;
    o_deviceFeatures->sparseBinding = features.sparseBinding;
    o_deviceFeatures->sparseResidencyBuffer = features.sparseResidencyBuffer;
    o_deviceFeatures->sparseResidencyImage2D = features.sparseResidencyImage2D;
    o_deviceFeatures->sparseResidencyImage3D = features.sparseResidencyImage3D;
    o_deviceFeatures->sparseResidency2Samples = features.sparseResidency2Samples;
    o_deviceFeatures->sparseResidency4Samples = features.sparseResidency4Samples;
    o_deviceFeatures->sparseResidency8Samples = features.sparseResidency8Samples;
    o_deviceFeatures->sparseResidency16Samples = features.sparseResidency16Samples;
    o_deviceFeatures->sparseResidencyAliased = features.sparseResidencyAliased;
    o_deviceFeatures->variableMultisampleRate = features.variableMultisampleRate;
    o_deviceFeatures->inheritedQueries = features.inheritedQueries;

    return features;
}

VkInstance _VKK_Internal_GetRawInstanceHandle(VKK_Instance instance) {
    return instance->handle;
}

VKK_Surface _VKK_Internal_WrapSurface(VkSurfaceKHR rawSurface) {
    VKK_Surface surface = malloc(sizeof(struct VKK_Surface_T));
    surface->handle = rawSurface;
    return surface;
}

void VKK_SetPushConstantData(void* data) {
    vkContext.pushConstantData = data;
    vkContext.pushConstantDataSet = true;
}

void VKK_Present(VKK_Color clearColor) {
    vkWaitForFences(vkContext.logicalDevice, 1, &vkContext.inFlightFences[vkContext.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(vkContext.logicalDevice, vkContext.swapchain.swapchainHandle, UINT64_MAX, vkContext.imageAvailableSemaphores[vkContext.currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        LogError("Failed to acquire swapchain image");
        return;
    }

    vkResetFences(vkContext.logicalDevice, 1, &vkContext.inFlightFences[vkContext.currentFrame]);

    vkResetCommandBuffer(vkContext.commandBuffers[imageIndex], 0);
    RecordCommandBuffer(vkContext.commandBuffers[imageIndex], imageIndex, clearColor);

    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkContext.imageAvailableSemaphores[vkContext.currentFrame],
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &vkContext.commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &vkContext.renderFinishedSemaphores[imageIndex]
    };

    if (vkQueueSubmit(vkContext.graphicsQueue, 1, &submitInfo, vkContext.inFlightFences[vkContext.currentFrame]) != VK_SUCCESS) {
        LogError("Failed to submit draw command buffer");
        return;
    }

    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vkContext.renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &vkContext.swapchain.swapchainHandle,
        .pImageIndices = &imageIndex
    };

    VkResult presentResult = vkQueuePresentKHR(vkContext.presentQueue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
        if (!RecreateSwapchain()) {
            LogError("Failed to recreate swapchain");
        }
    } else if (presentResult != VK_SUCCESS) {
        LogError("Failed to present swapchain image");
    }

    ProcessPendingDeletions();

    vkContext.currentFrame = (vkContext.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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

static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
    
    if (!CreateImage(width, height, textureFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, &texture->image, &texture->memory)) {
        free(texture);
        VKK_DestroyBuffer(stagingBuffer);
        return NULL;
    }
    
    TransitionImageLayout(texture->image, textureFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    CopyBufferToImage(stagingBuffer->handle, texture->image, width, height);
    
    TransitionImageLayout(texture->image, textureFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    VKK_DestroyBuffer(stagingBuffer);

    const VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = texture->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = textureFormat,
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

uint32_t VKK_EnumeratePhysicalDevices(VKK_PhysicalDeviceInfo *o_devices, uint32_t maxDevices) {

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, NULL);

    VkPhysicalDevice devices[16];
    deviceCount = deviceCount > 16 ? 16 : deviceCount;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, devices);

    uint32_t count = deviceCount > maxDevices ? maxDevices : deviceCount;

    for (uint32_t i = 0; i < count; i++) {
        FillPhysicalDeviceProperties(devices[i], &o_devices[i].properties);
        FillPhysicalDeviceFeatures(devices[i], &o_devices[i].features);
    }

    memcpy(vkContext.availablePhysicalDevices, devices, count * sizeof(VkPhysicalDevice));
    vkContext.availableDeviceCount = count;

    return count;
}

void VKK_SetSurface(VKK_Surface surface, uint32_t width, uint32_t height) {
    vkContext.surface = surface->handle;

    vkContext.windowWidth = width;
    vkContext.windowHeight = height;
}

VKK_Result VKK_InitInstance(VKK_Config config, VKK_InstanceInfo* o_instanceInfo) {

    logWarnings = config.logWarnings;

    PREFERRED_PRESENT_MODE = config.presentMode;
    DESIRED_IMAGE_COUNT = config.imageBufferSize;

    if (config.enableValidationLayers) {
        const char* layers[] = { 
            "VK_LAYER_KHRONOS_validation"
        };
        vkContext.layers = layers;
        vkContext.layersAmount = 1;
    } else {
        vkContext.layers = NULL;
        vkContext.layersAmount = 0;
    }

    vkContext.extensionsAmount = config.requiredExtensionsCount;
    vkContext.extensions = config.requiredExtensions;

    if (!CreateInstance(o_instanceInfo)) {
        return VKK_ERROR_INSTANCE_CREATION_FAILED;
    }

    return VKK_SUCCESS;
}

VKK_Result VKK_InitDevice(uint32_t deviceIndex, VKK_PhysicalDeviceInfo* o_deviceInfo) {

    if (deviceIndex >= vkContext.availableDeviceCount) {
        return VKK_ERROR_INVALID_DEVICE_INDEX;
    }

    vkContext.physicalDevice = vkContext.availablePhysicalDevices[deviceIndex];
    VkPhysicalDeviceProperties properties = FillPhysicalDeviceProperties(vkContext.physicalDevice, &o_deviceInfo->properties);
    VkPhysicalDeviceFeatures features = FillPhysicalDeviceFeatures(vkContext.physicalDevice, &o_deviceInfo->features);

    if (!FindQueueFamilies()) {
        return VKK_ERROR_NO_SUITABLE_DEVICE;
    }

    if (!CreateLogicalDevice(features)) {
        return VKK_ERROR_DEVICE_CREATION_FAILED;
    }

    if (!CreateVkSwapchain(&vkContext.swapchain)) {
        return VKK_ERROR_SWAPCHAIN_CREATION_FAILED;
    }

    if (!CreateDepthResources()) {
        return VKK_ERROR_DEPTH_RESOURCE_CREATION_FAILED;
    }

    if (!CreateRenderPass()) {
        return VKK_ERROR_RENDER_PASS_CREATION_FAILED;
    }

    return VKK_SUCCESS;
}

VKK_Result VKK_InitRenderer(VKK_RendererConfig rendererConfig) {

    vkContext.pushConstantRange = rendererConfig.pushConstantRange;

    uint32_t maxSets = rendererConfig.maxDescriptorSets > 0 ? rendererConfig.maxDescriptorSets : 16;

    if (!vkContext.descriptorSetLayout) {
        LogError("VKK_CreateDescriptorSetLayout has to be called before calling VKK_InitRenderer");
        return VKK_ERROR_WRONG_EXECUTION_ORDER;
    }

    vkContext.pipelineFinalized = true;

    if (!CreateFrameBuffers(&vkContext.swapchain)) {
        return VKK_ERROR_FRAMEBUFFER_CREATION_FAILED;
    }

    if (!CreateCommandPool()) {
        return VKK_ERROR_COMMAND_POOL_CREATION_FAILED;
    }

    if (!CreateCommandBuffers()) {
        return VKK_ERROR_COMMAND_BUFFER_CREATION_FAILED;
    }

    if (!CreateSyncObjects()) {
        return VKK_ERROR_SYNC_OBJECTS_CREATION_FAILED;
    }

    if (!CreateDescriptorPoolAndSet(maxSets)) {
        return VKK_ERROR_DESCRIPTOR_POOL_CREATION_FAILED;
    }

    vkContext.currentFrame = 0;

    return VKK_SUCCESS;
}

void VKK_End() {
    vkDeviceWaitIdle(vkContext.logicalDevice);

    CleanupSwapchain();

    vkDestroyDescriptorPool(vkContext.logicalDevice, vkContext.descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(vkContext.logicalDevice, vkContext.descriptorSetLayout, NULL);

    for (uint32_t i = 0; i < vkContext.pendingDeletionCount; i++) {
        HandleDeletion(&vkContext.pendingDeletions[i]);
    }

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkContext.logicalDevice, vkContext.imageAvailableSemaphores[i], NULL);
        vkDestroyFence(vkContext.logicalDevice, vkContext.inFlightFences[i], NULL);
    }

    for (uint32_t i = 0; i < vkContext.swapchain.imageCount; i++) {
        vkDestroySemaphore(vkContext.logicalDevice, vkContext.renderFinishedSemaphores[i], NULL);
    }

    free(vkContext.renderFinishedSemaphores);

    if (vkContext.depthImageView != VK_NULL_HANDLE)
        vkDestroyImageView(vkContext.logicalDevice, vkContext.depthImageView, NULL);

    if (vkContext.depthImage != VK_NULL_HANDLE)
        vkDestroyImage(vkContext.logicalDevice, vkContext.depthImage, NULL);

    if (vkContext.depthMemory != VK_NULL_HANDLE)
        vkFreeMemory(vkContext.logicalDevice, vkContext.depthMemory, NULL);

    vkDestroyCommandPool(vkContext.logicalDevice, vkContext.commandPool, NULL);
    free(vkContext.commandBuffers);

    vkDestroyRenderPass(vkContext.logicalDevice, vkContext.renderPass, NULL);
    
    vkDestroyDevice(vkContext.logicalDevice, NULL);
    vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, NULL);
    vkDestroyInstance(vkContext.instance, NULL);
}
