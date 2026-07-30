#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "include/shared.h"
#include "include/vkKatzi.h"
#include "include/logger.h"

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

#define MAX_PENDING_DELETIONS 1024

typedef struct {
    VKK_Buffer buffer;
    uint32_t framesUntilDeletion;
} PendingDeletion;


typedef struct {
    VkBuffer vertexBuffer;
    uint32_t vertexCount;
    VkBuffer indexBuffer;
    uint32_t indexCount;
    VKK_Pipeline pipeline;
} DrawCall;

#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_DRAW_CALLS 1024
#define MAX_UNIFORMS 32

typedef struct  {
    VkInstance instance;
    const char** layers;
    const char** extensions;
    uint32_t layersAmount;
    uint32_t extensionsAmount;

    VkSurfaceKHR surface;

    GLFWwindow* window;
    int windowWidth;
    int windowHeight;

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

    bool frameBufferResized;

    VKK_Uniform uniforms[MAX_UNIFORMS];
    uint32_t uniformCount;
    bool pipelineFinalized;

    VKK_PushConstantRange pushConstantRange;
    void* pushConstantData;
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
    uint32_t binding;
    VkShaderStageFlags stageFlags;
};

struct VKK_Pipeline_T {
    VkPipeline handle;
    VkPipelineLayout layout;
};


static VkContext vkContext;
static bool verboseLogging = false;

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

    return true;
}

static bool CreateVkSurface() {
    glfwCreateWindowSurface(vkContext.instance, vkContext.window, NULL, &vkContext.surface);

    if (!vkContext.surface) {
        return false;
    }

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

static bool CreateLogicalDevice() {

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
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
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
    o_info->surfacePresentModes = calloc(o_info->formatCount, sizeof(*o_info->surfacePresentModes));

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

    // TODO: Make this a warning
    Log("Preferred present mode isn't supported, defaulting to FIFO", true);
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

    uint32_t imageCount;
    if (DESIRED_IMAGE_COUNT >= info.surfaceCapabilities.minImageCount) {
        imageCount = DESIRED_IMAGE_COUNT;
    } else {
        // TODO: Make these logs warnings
        Log("Desired image buffer size was too low; using min image count", true);
        imageCount = info.surfaceCapabilities.minImageCount;
    }

    if (info.surfaceCapabilities.maxImageCount > 0 && imageCount > info.surfaceCapabilities.maxImageCount) {
        Log("Desired image count was too high; using max image count", true);
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

        if (!vkCreateImageView(vkContext.logicalDevice, &info, NULL, &o_swapchain->imageViews[i]) == VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

static bool CreateRenderPass() {

    const VkAttachmentDescription colorAttachment = {
        .format = vkContext.swapchain.swapchainFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    const VkAttachmentReference colorAttachmentReference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    const VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentReference
    };

    const VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    const VkRenderPassCreateInfo renderPassCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
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
            swapchain->imageViews[i]
        };

        const VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = vkContext.renderPass,
            .attachmentCount = 1,
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

void VKK_DestroyBuffer(VKK_Buffer buffer) {
    if (!buffer)
        return;

    if (vkContext.pendingDeletionCount >= MAX_PENDING_DELETIONS) {
        // TODO: Make this a warning maybe? Ill figure it out
        Log("Max pending deletions exceeded", true);
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
        Log("VKK_WriteBuffer: buffer is NULL", true);
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

VKK_Uniform VKK_CreateUniform(uint32_t binding, size_t size, VKK_ShaderStage stage) { 

    if (vkContext.pipelineFinalized) {
        fprintf(stderr, "[VKK][ERROR]: VKK_CreateUniform: Uniforms must be created before VKK_InitPipeline finishes\n");
        return NULL;
    }

    if (vkContext.uniformCount >= MAX_UNIFORMS) {
        fprintf(stderr, "[VKK][ERROR]: Max uniforms (%d) exceeded\n", MAX_UNIFORMS);
        return NULL;
    }

    VKK_Uniform uniform = malloc(sizeof(struct VKK_Uniform_T));
    uniform->buffer = VKK_CreateBuffer(size, VKK_BUFFER_USAGE_UNIFORM);
    uniform->binding = binding;
    uniform->stageFlags = ConvertShaderStage(stage);

    if (!uniform->buffer) {
        free(uniform);
        return NULL;
    }

    vkContext.uniforms[vkContext.uniformCount++] = uniform;
    return uniform;
}

void VKK_WriteUniform(VKK_Uniform uniform, const void* data, size_t size, size_t offset) {
    if (!uniform)
        return;

    VKK_WriteBuffer(uniform->buffer, data, size, offset);
}

static VkFormat ConvertVertexFormat(VKK_VertexFormat format) {
    switch (format) {
        case VKK_VERTEX_FORMAT_FLOAT2:
            return VK_FORMAT_R32G32_SFLOAT;

        case VKK_VERTEX_FORMAT_FLOAT3:
            return VK_FORMAT_R32G32B32_SFLOAT;

        case VKK_VERTEX_FORMAT_FLOAT4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    return VK_FORMAT_R32G32_SFLOAT;
}

VKK_Pipeline VKK_CreatePipeline(VKK_PipelineDescription desc) {

    VkShaderModule vertModule = CreateShaderModule(desc.shaderPaths.vertexShaderPath);
    VkShaderModule fragModule = CreateShaderModule(desc.shaderPaths.fragmentShaderPath);

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

    VkVertexInputBindingDescription bindingDescription = {
        .binding = 0,
        .stride = desc.vertexStride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX   
    };

    VkVertexInputAttributeDescription attributeDescriptions[16];

    for (uint32_t i = 0; i < desc.attributeCount; i++) {
        attributeDescriptions[i] = (VkVertexInputAttributeDescription){
            .binding = 0,
            .location = desc.attributes[i].location,
            .format = ConvertVertexFormat(desc.attributes[i].format),
            .offset = desc.attributes[i].offset
        };
    }

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = desc.attributeCount,
        .pVertexAttributeDescriptions = attributeDescriptions
    };

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = topology,
    };

    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
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
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f   
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
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    VKK_Pipeline pipeline = malloc(sizeof(struct VKK_Pipeline_T));

    if (vkCreatePipelineLayout(vkContext.logicalDevice, &layoutInfo, NULL, &pipeline->layout) != VK_SUCCESS) {
        free(pipeline);
        return false;
    }

    const VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &colorBlending,
        .layout = pipeline->layout,
        .renderPass = vkContext.renderPass,
        .subpass = 0,
        .pDynamicState = &dynamicState
    };

    if (vkCreateGraphicsPipelines(vkContext.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline->handle) != VK_SUCCESS) {
        vkDestroyPipelineLayout(vkContext.logicalDevice, pipeline->layout, NULL);
        free(pipeline);
        return false;
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

static bool RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        return false;
    }

    const VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vkContext.renderPass,
        .framebuffer = vkContext.swapchain.framebuffers[imageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = vkContext.swapchain.dimensions
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor
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
        .offset = { 0, 0},
        .extent = vkContext.swapchain.dimensions
    };
    
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdPushConstants(commandBuffer, vkContext.drawCalls[0].pipeline->layout, ConvertShaderStage(vkContext.pushConstantRange.shaderStage), vkContext.pushConstantRange.offset, vkContext.pushConstantRange.size, vkContext.pushConstantData);

    if (vkContext.drawCallIndex > 0) {

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.drawCalls[0].pipeline->layout, 0, 1, &vkContext.descriptorSet, 0, NULL);

        for (uint32_t i = 0; i < vkContext.drawCallIndex; i++) {
            
            DrawCall* drawCall = &vkContext.drawCalls[i];

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCall->pipeline->handle);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &drawCall->vertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, drawCall->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, drawCall->pipeline->layout, 0, 1, &vkContext.descriptorSet, 0, NULL);
        
            vkCmdDrawIndexed(commandBuffer, drawCall->indexCount, 1, 0, 0, 0);
        }

    }

    vkContext.drawCallIndex = 0;

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        return false;
    }

    return true;
}

void VKK_Draw(VKK_Pipeline pipeline, VKK_Buffer vertexBuffer, uint32_t vertexCount, VKK_Buffer indexBuffer, uint32_t indexCount) {

    if (vkContext.drawCallIndex >= MAX_DRAW_CALLS) {
        fprintf(stderr, "[VKK][ERROR]: Max draw calls (%d) in frame reached, dropping draw call\n", MAX_DRAW_CALLS);
        return;
    }

    DrawCall drawCall = {
        .vertexBuffer = vertexBuffer->handle,
        .indexBuffer = indexBuffer->handle,
        .vertexCount = vertexCount,
        .indexCount = indexCount,
        .pipeline = pipeline
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

    Log("Failed to find suitable memory type", true);
    return UINT32_MAX;
}

// TODO: Implement VKK_Result with this
static bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* o_buffer, VkDeviceMemory* o_bufferMemory) {
    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(vkContext.logicalDevice, &bufferInfo, NULL, o_buffer) != VK_SUCCESS) {
        Log("Failed to create buffer", true);
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
        Log("Failed to allocate buffer memory", true);
        return false;
    }

    vkBindBufferMemory(vkContext.logicalDevice, *o_buffer, *o_bufferMemory, 0);

    return true;
}

static bool CreateDescriptorSetLayout() {

    VkDescriptorSetLayoutBinding bindings[MAX_UNIFORMS];

    for (uint32_t i = 0; i < vkContext.uniformCount; i++) {
        bindings[i] = (VkDescriptorSetLayoutBinding){
            .binding = vkContext.uniforms[i]->binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = vkContext.uniforms[i]->stageFlags
        };
    }

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = vkContext.uniformCount,
        .pBindings = bindings
    };

    if (vkCreateDescriptorSetLayout(vkContext.logicalDevice, &layoutInfo, NULL, &vkContext.descriptorSetLayout) != VK_SUCCESS) {
        return false;
    }

    return true;
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

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(vkContext.window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(vkContext.window, &width, &height);
        glfwWaitEvents();
    }

    vkContext.windowWidth = width;
    vkContext.windowHeight = height;

    vkDeviceWaitIdle(vkContext.logicalDevice);

    CleanupSwapchain();

    if (!CreateVkSwapchain(&vkContext.swapchain)) {
        return false;
    }

    if (!CreateFrameBuffers(&vkContext.swapchain)) {
        return false;
    }

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
        Log("VKK_CreateBuffer: Failed to create buffer", true);
        free(buffer);
        return NULL;
    }

    return buffer;
}

static bool CreateDescriptorPoolAndSet() {

    const VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = vkContext.uniformCount
    };

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
        .maxSets = 1
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

    VkDescriptorBufferInfo bufferInfos[MAX_UNIFORMS];
    VkWriteDescriptorSet writes[MAX_UNIFORMS];

    for (uint32_t i = 0; i < vkContext.uniformCount; i++) {
        bufferInfos[i] = (VkDescriptorBufferInfo){
            .buffer = vkContext.uniforms[i]->buffer->handle,
            .offset = 0,
            .range = vkContext.uniforms[i]->buffer->size
        };

        writes[i] = (VkWriteDescriptorSet){
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vkContext.descriptorSet,
            .dstBinding = vkContext.uniforms[i]->binding,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfos[i]
        };
    }

    vkUpdateDescriptorSets(vkContext.logicalDevice, vkContext.uniformCount, writes, 0, NULL);

    return true;
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    VkContext* vkContext = (VkContext*)glfwGetWindowUserPointer(window);
    vkContext->frameBufferResized = true;
}

static void DestroyBuffer(VKK_Buffer buffer) {
    vkDestroyBuffer(vkContext.logicalDevice, buffer->handle, NULL);
    vkFreeMemory(vkContext.logicalDevice, buffer->memory, NULL);
    free(buffer);
}

static void ProcessPendingDeletions() {
    for (uint32_t i = 0; i < vkContext.pendingDeletionCount;) {
        vkContext.pendingDeletions[i].framesUntilDeletion--;

        if (vkContext.pendingDeletions[i].framesUntilDeletion <= 0) {
            VKK_Buffer buffer = vkContext.pendingDeletions[i].buffer;

            DestroyBuffer(buffer);

            vkContext.pendingDeletions[i] = vkContext.pendingDeletions[--vkContext.pendingDeletionCount];
        } else {
            i++;
        }
    }
}

static void ProcessPendingDeletionsImmediate() {
    for (uint32_t i = 0; i < vkContext.pendingDeletionCount; i++) {
        VKK_Buffer buffer = vkContext.pendingDeletions[i].buffer;
        DestroyBuffer(buffer);
    }
}


static void FillPhysicalDeviceInfo(VkPhysicalDevice device, VKK_PhysicalDeviceInfo* o_deviceInfo) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    strncpy(o_deviceInfo->name, properties.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    o_deviceInfo->apiVersion = properties.apiVersion;
    o_deviceInfo->driverVersion = properties.driverVersion;
    o_deviceInfo->vendorID = properties.vendorID;
    o_deviceInfo->deviceID = properties.deviceID;
    o_deviceInfo->deviceType = properties.deviceType;

}

void VKK_SetPushConstantData(void* data) {
    vkContext.pushConstantData = data;
}

void VKK_Present() {
    vkWaitForFences(vkContext.logicalDevice, 1, &vkContext.inFlightFences[vkContext.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(vkContext.logicalDevice, vkContext.swapchain.swapchainHandle, UINT64_MAX, vkContext.imageAvailableSemaphores[vkContext.currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        Log("Failed to acquire swapchain image", true);
        return;
    }

    vkResetFences(vkContext.logicalDevice, 1, &vkContext.inFlightFences[vkContext.currentFrame]);

    vkResetCommandBuffer(vkContext.commandBuffers[imageIndex], 0);
    RecordCommandBuffer(vkContext.commandBuffers[imageIndex], imageIndex);

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
        Log("Failed to submit draw command buffer", true);
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

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || vkContext.frameBufferResized) {
        vkContext.frameBufferResized = false;
        RecreateSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        Log("Failed to present swapchain image", true);
    }

    ProcessPendingDeletions();

    vkContext.currentFrame = (vkContext.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

uint32_t VKK_EnumeratePhysicalDevices(VKK_PhysicalDeviceInfo *o_devices, uint32_t maxDevices) {

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, NULL);

    VkPhysicalDevice devices[16];
    deviceCount = deviceCount > 16 ? 16 : deviceCount;
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, devices);

    uint32_t count = deviceCount > maxDevices ? maxDevices : deviceCount;

    for (uint32_t i = 0; i < count; i++) {
        FillPhysicalDeviceInfo(devices[i], &o_devices[i]);
    }

    memcpy(vkContext.availablePhysicalDevices, devices, count * sizeof(VkPhysicalDevice));
    vkContext.availableDeviceCount = count;

    return count;
}

VKK_Result VKK_InitInstance(GLFWwindow* window, VKK_Config config, VKK_InstanceInfo* o_instanceInfo) {

    verboseLogging = config.verboseLogging;

    PREFERRED_PRESENT_MODE = config.presentMode;
    DESIRED_IMAGE_COUNT = config.imageBufferSize;

    glfwSetWindowUserPointer(window, &vkContext);
    glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

    vkContext.window = window;

    glfwGetFramebufferSize(window, &vkContext.windowWidth, &vkContext.windowHeight);

    uint32_t instanceExtensionsAmount;
    const char** instanceExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionsAmount);

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

    vkContext.extensionsAmount = instanceExtensionsAmount;
    vkContext.extensions = instanceExtensions;


    if (!CreateInstance(o_instanceInfo)) {
        return VKK_ERROR_INSTANCE_CREATION_FAILED;
    }

    if (!CreateVkSurface()) {
        return VKK_ERROR_SURFACE_CREATION_FAILED;
    }

    return VKK_SUCCESS;
}

VKK_Result VKK_InitDevice(uint32_t deviceIndex, VKK_PhysicalDeviceInfo* o_deviceInfo) {

    if (deviceIndex >= vkContext.availableDeviceCount) {
        return VKK_ERROR_INVALID_DEVICE_INDEX;
    }

    vkContext.physicalDevice = vkContext.availablePhysicalDevices[deviceIndex];
    FillPhysicalDeviceInfo(vkContext.physicalDevice, o_deviceInfo);

    if (!FindQueueFamilies()) {
        return VKK_ERROR_NO_SUITABLE_DEVICE;
    }

    if (!CreateLogicalDevice()) {
        return VKK_ERROR_DEVICE_CREATION_FAILED;
    }

    if (!CreateVkSwapchain(&vkContext.swapchain)) {
        return VKK_ERROR_SWAPCHAIN_CREATION_FAILED;
    }

    if (!CreateRenderPass()) {
        return VKK_ERROR_RENDER_PASS_CREATION_FAILED;
    }

    return VKK_SUCCESS;
}

VKK_Result VKK_InitPipeline(VKK_PushConstantRange pushConstantRangeConfig) {

    vkContext.pushConstantRange = pushConstantRangeConfig;

    if (!CreateDescriptorSetLayout()) {
        return VKK_ERROR_DESCRIPTOR_SET_LAYOUT_CREATION_FAILED;
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

    if (!CreateDescriptorPoolAndSet()) {
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

    ProcessPendingDeletionsImmediate();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkContext.logicalDevice, vkContext.imageAvailableSemaphores[i], NULL);
        vkDestroyFence(vkContext.logicalDevice, vkContext.inFlightFences[i], NULL);
    }

    for (uint32_t i = 0; i < vkContext.swapchain.imageCount; i++) {
        vkDestroySemaphore(vkContext.logicalDevice, vkContext.renderFinishedSemaphores[i], NULL);
    }

    free(vkContext.renderFinishedSemaphores);

    vkDestroyCommandPool(vkContext.logicalDevice, vkContext.commandPool, NULL);
    free(vkContext.commandBuffers);

    vkDestroyRenderPass(vkContext.logicalDevice, vkContext.renderPass, NULL);
    
    vkDestroyDevice(vkContext.logicalDevice, NULL);
    vkDestroySurfaceKHR(vkContext.instance, vkContext.surface, NULL);
    vkDestroyInstance(vkContext.instance, NULL);
}
