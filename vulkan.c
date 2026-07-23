#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "include/vulkan.h"
#include "include/shared.h"
#include "include/rectangle.h"

const VkPresentModeKHR PREFERRED_PRESENT_MODE = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
const uint32_t DESIRED_IMAGE_COUNT = 2;

typedef struct {
    VkSurfaceFormatKHR* surfaceFormats;
    uint32_t formatCount;
    VkPresentModeKHR* surfacePresentModes;
    uint32_t presentModesCount;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
} VkSwapchainInfo;

typedef struct {
    float position[2];
} Vertex;

typedef struct {
    float ortho[16];
} UniformBufferObject;

typedef struct {
    float offset[2];
    float scale[2];
    float color[3];
} RectangleInstance;

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

    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;

    int32_t graphicsQueueFamilyIndex;
    int32_t presentQueueFamilyIndex;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchain swapchain;

    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
    uint32_t instanceCapacity;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapped;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;

    RectangleInstance* rectangles;
    uint32_t rectangleCount;

    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;

    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore* renderFinishedSemaphores;
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

    uint32_t currentFrame;

    bool frameBufferResized;
} VkContext;

static const Vertex vertices[] = {
    {{-1.0f, -1.0f}},
    {{1.0f, -1.0f}},
    {{1.0f, 1.0f}},
    {{-1.0f, 1.0f}},
};

static const uint16_t indices[] = {
    0, 1, 2, 
    0, 2, 3
};

#define VERTEX_COUNT (sizeof(vertices) / sizeof(vertices[0]))
#define INDEX_COUNT (sizeof(indices) / sizeof(indices[0]))

static VkContext vkContext;

static bool CreateInstance() {

    const VkApplicationInfo applicationInfo = {
        .apiVersion = VK_API_VERSION_1_4,
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
        fprintf(stderr, "Failed to create Vulkan instance\n");
        return false;
    }

    fprintf(stdout, "Created Vulkan Instance\n");

    return true;
}

static bool CreateVkSurface() {
    glfwCreateWindowSurface(vkContext.instance, vkContext.window, NULL, &vkContext.surface);

    if (!vkContext.surface) {
        fprintf(stderr, "Failed to create Vulkan surface\n");
        return false;
    }

    fprintf(stdout, "Created Vulkan Surface\n");

    return true;
}

static bool GetPhysicalDevice() {
    uint32_t deviceCount;

    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, NULL);
    
    if (deviceCount == 0) {
        fprintf(stderr, "No GPUs with Vulkan support found\n");
        return false;
    }

    VkPhysicalDevice physicalDevices[8];
    vkEnumeratePhysicalDevices(vkContext.instance, &deviceCount, physicalDevices);

    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDevice device = physicalDevices[i];

        uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, NULL);

        VkQueueFamilyProperties properties[8];
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, properties);

        int32_t graphicsQueueFamilyIndex = -1;
        int32_t presentQueueFamilyIndex = -1;

        for (uint32_t j = 0; j < queueCount; j++) {
            if (properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueFamilyIndex = j;

                VkBool32 supported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, j, vkContext.surface, &supported);

                if (supported) {
                    presentQueueFamilyIndex = j;

                    if (graphicsQueueFamilyIndex != -1)
                        break;
                }
            }
        }

        if (graphicsQueueFamilyIndex != -1 && presentQueueFamilyIndex != -1) {
            vkContext.physicalDevice = device;
            vkContext.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
            vkContext.presentQueueFamilyIndex = presentQueueFamilyIndex;

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            fprintf(stdout, "Got physical device: (Name: %s, API version: %i, Driver version: %i)\n", properties.deviceName, properties.apiVersion, properties.driverVersion);

            return true;
        }
    }

    fprintf(stderr, "Failed to get physical device");

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
        fprintf(stderr, "Failed to create Vulkan physical device\n");
        return false;
    }

    vkGetDeviceQueue(vkContext.logicalDevice, vkContext.graphicsQueueFamilyIndex, 0, &vkContext.graphicsQueue);
    vkGetDeviceQueue(vkContext.logicalDevice, vkContext.presentQueueFamilyIndex, 0, &vkContext.presentQueue);

    fprintf(stdout, "Created Vulkan logical device\n");

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

static void LogPresentMode(VkPresentModeKHR presentMode) {
    switch (presentMode) {
        case VK_PRESENT_MODE_FIFO_KHR:
            fprintf(stdout, "Using present mode: FIFO_KHR\n");
            break;

        case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT:
            fprintf(stdout, "Using present mode: FIFO_LATEST_READY_(EXT/KHR)\n");
            break;

        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            fprintf(stdout, "Using present mode: FIFO_RELAXED_KHR\n");
            break;

        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            fprintf(stdout, "Using present mode: IMMEDIATE_KHR\n");
            break;

        case VK_PRESENT_MODE_MAILBOX_KHR:
            fprintf(stdout, "Using present mode: MAILBOX_KHR\n");
            break;

        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            fprintf(stdout, "Using present mode: SHARED_CONTINUOUS_REFRESH_KHR\n");
            break;

        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            fprintf(stdout, "Using present mode: SHARED_DEMAND_REFRESH\n");
            break;
    }
}

static bool CreateVkSwapchain(VkSwapchain* o_swapchain) {

    VkSwapchainInfo info;
    GetVkSwapchainInfo(&info);

    VkSurfaceFormatKHR format = GetVkSwapchainFormat(info.surfaceFormats, info.formatCount);
    VkPresentModeKHR presentMode = GetVkSwapchainPresentMode(info.surfacePresentModes, info.presentModesCount);

    LogPresentMode(presentMode);

    VkExtent2D extent = GetVkSwapchainExtent(&info.surfaceCapabilities, vkContext.windowWidth, vkContext.windowHeight);

    uint32_t imageCount;
    if (DESIRED_IMAGE_COUNT >= info.surfaceCapabilities.minImageCount) {
        imageCount = DESIRED_IMAGE_COUNT;
    } else {
        fprintf(stderr, "Desired image count was too low; using min image count\n");
        imageCount = info.surfaceCapabilities.minImageCount;
    }

    fprintf(stdout, "Using %d images for buffering\n", imageCount);
    fprintf(stdout, "Min image count: %d\n", info.surfaceCapabilities.minImageCount);
    fprintf(stdout, "Max image count: %d\n", info.surfaceCapabilities.maxImageCount);

    if (info.surfaceCapabilities.maxImageCount > 0 && imageCount > info.surfaceCapabilities.maxImageCount) {
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
        fprintf(stderr, "Failed to create Vulkan swapchain\n");
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
            fprintf(stderr, "Failed to create Vulkan swapchain\n");
            return false;
        }
    }

    fprintf(stdout, "Created Vulkan swapchain\n");

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
        fprintf(stderr, "Failed to create Vulkan render pass\n");
        return false;
    }

    fprintf(stdout, "Created Vulkan render pass\n");

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
            fprintf(stderr, "Failed to create Vulkan Framebuffer %u\n", i);
            return false;
        }

    }

    fprintf(stdout, "Created %u Vulkan framebuffers\n", swapchain->imageCount);

    return true;
}

static bool CreateCommandPool() {
    const VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vkContext.graphicsQueueFamilyIndex
    };

    if (vkCreateCommandPool(vkContext.logicalDevice, &poolInfo, NULL, &vkContext.commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan Command pool\n");
        return false;
    }

    fprintf(stdout, "Created Vulkan command pool\n");

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
        fprintf(stderr, "Failed to allocate Vulkan command buffers\n");
        return false;
    }

    fprintf(stdout, "Allocated %u Vulkan command buffers\n", vkContext.swapchain.imageCount);

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
        fprintf(stderr, "Failed to create Vulkan shader module: %s\n", path);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}


//probably describes how to bind a single vertex with its size and stuff idrk
static VkVertexInputBindingDescription GetVertexBindingDescription(void) {
    return (VkVertexInputBindingDescription) {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

//Describes what attributes (variables) of the Vertex struct are what
static void GetVertexAttributeDescriptions(VkVertexInputAttributeDescription* o_attributes) {
    o_attributes[0] = (VkVertexInputAttributeDescription) {
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };

    //o_attributes[1] = (VkVertexInputAttributeDescription) {
    //    .binding = 0,
    //    .location = 1,
    //    .format = VK_FORMAT_R32G32B32_SFLOAT,
    //    .offset = offsetof(Vertex, color)
    //};
}

static VkVertexInputBindingDescription GetBindingDescription(VkVertexInputBindingDescription* o_bindings) {
    o_bindings[0] = (VkVertexInputBindingDescription){
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    o_bindings[1] = (VkVertexInputBindingDescription){
        .binding = 1,
        .stride = sizeof(RectangleInstance),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
    };
}

static void GetAttributeDescriptions(VkVertexInputAttributeDescription* o_attributes) {
    o_attributes[0] = (VkVertexInputAttributeDescription){
        .binding = 0,
        .location = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, position)
    };

    o_attributes[1] = (VkVertexInputAttributeDescription){
        .binding = 1,
        .location = 1,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(RectangleInstance, offset)
    };

    o_attributes[2] = (VkVertexInputAttributeDescription){
        .binding = 1,
        .location = 2,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(RectangleInstance, scale)
    };

    o_attributes[3] = (VkVertexInputAttributeDescription){
        .binding = 1,
        .location = 3,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(RectangleInstance, color)
    };
}

static bool CreateGraphicsPipeline() {

    VkShaderModule vertModule = CreateShaderModule("/media/Vulkan/VkKatzi/shader/compiled/vert.spv");
    VkShaderModule fragModule = CreateShaderModule("/media/Vulkan/VkKatzi/shader/compiled/frag.spv");

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

    //const VkVertexInputBindingDescription bindingDescription = GetVertexBindingDescription();

    //VkVertexInputAttributeDescription attributeDescriptions[2];
    //GetVertexAttributeDescriptions(attributeDescriptions);

    VkVertexInputBindingDescription bindingDescriptions[2];
    GetBindingDescription(bindingDescriptions);

    VkVertexInputAttributeDescription attributeDescription[4];
    GetAttributeDescriptions(attributeDescription);

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 2,
        .pVertexBindingDescriptions = bindingDescriptions,
        .vertexAttributeDescriptionCount = 4,
        .pVertexAttributeDescriptions = attributeDescription
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    //const VkViewport viewport = {
    //    .x = 0.0f,
    //    .y = 0.0f,
    //    .width = (float)vkContext->swapchain.dimensions.width,
    //    .height = (float)vkContext->swapchain.dimensions.height,
    //    .minDepth = 0.0f,
    //    .maxDepth = 1.0f
    //};

    //const VkRect2D scissor = {
    //    .offset = {0, 0},
    //    .extent = vkContext->swapchain.dimensions
    //};

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
        .blendEnable = VK_FALSE
    };

    const VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    const VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vkContext.descriptorSetLayout
    };

    if (vkCreatePipelineLayout(vkContext.logicalDevice, &layoutInfo, NULL, &vkContext.pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
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
        .layout = vkContext.pipelineLayout,
        .renderPass = vkContext.renderPass,
        .subpass = 0,
        .pDynamicState = &dynamicState
    };

    if (vkCreateGraphicsPipelines(vkContext.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkContext.graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        return false;
    }

    vkDestroyShaderModule(vkContext.logicalDevice, vertModule, NULL);
    vkDestroyShaderModule(vkContext.logicalDevice, fragModule, NULL);

    fprintf(stdout, "Created Vulkan graphics pipeline\n");

    return true;
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
                fprintf(stderr, "Failed to create sync objects for frame %u\n", i);
                return false;
            }
    }

    vkContext.renderFinishedSemaphores = calloc(vkContext.swapchain.imageCount, sizeof(VkSemaphore));

    for (uint32_t i = 0; i < vkContext.swapchain.imageCount; i++) {
        if (vkCreateSemaphore(vkContext.logicalDevice, &semaphoreInfo, NULL, &vkContext.renderFinishedSemaphores[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create render finished semaphores for image %u\n", i);
            return false;
        }
    }


    fprintf(stdout, "Created Vulkan sync objects\n");

    return true;
}

static bool RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        fprintf(stderr, "Failed to begin recording command buffer\n");
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
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.graphicsPipeline);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)vkContext.swapchain.dimensions.width,
        .height = (float)vkContext.swapchain.dimensions.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { 0, 0},
        .extent = vkContext.swapchain.dimensions
    };

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    if (vkContext.rectangleCount > 0) {
        VkBuffer vertexBuffers[] = { vkContext.vertexBuffer, vkContext.instanceBuffer };
        VkDeviceSize offsets[] = { 0, 0 };

        vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, vkContext.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkContext.pipelineLayout, 0, 1, &vkContext.descriptorSet, 0, NULL);
    
        vkCmdDrawIndexed(commandBuffer, INDEX_COUNT, vkContext.rectangleCount, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command buffer\n");
        return false;
    }

    return true;
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

    fprintf(stderr, "Failed to find suitable memory type\n");
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
        fprintf(stderr, "Failed to create buffer\n");
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
        fprintf(stderr, "Failed to allocate buffer memory\n");
        return false;
    }

    vkBindBufferMemory(vkContext.logicalDevice, *o_buffer, *o_bufferMemory, 0);

    return true;
}

static bool CreateVertexBuffer() {

    const VkDeviceSize bufferSize = sizeof(vertices);

    if (!CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       &vkContext.vertexBuffer, &vkContext.vertexBufferMemory)) {
        return false;
    }

    void* data;
    vkMapMemory(vkContext.logicalDevice, vkContext.vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(vkContext.logicalDevice, vkContext.vertexBufferMemory);

    fprintf(stdout, "Created Vulkan vertex buffer\n");

    return true;
}

static bool CreateIndexBuffer() {
    const VkDeviceSize bufferSize = sizeof(indices);

    if (!CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vkContext.indexBuffer, &vkContext.indexBufferMemory)) {
        return false; 
    }

    void* data;
    vkMapMemory(vkContext.logicalDevice, vkContext.indexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices, (size_t)bufferSize);
    vkUnmapMemory(vkContext.logicalDevice, vkContext.indexBufferMemory);

    fprintf(stdout, "Created Vulkan index buffer\n");

    return true;
}

static bool CreateInstanceBuffer(uint32_t maxInstances) {
    VkDeviceSize bufferSize = sizeof(RectangleInstance) * maxInstances;

    if (!CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vkContext.instanceBuffer, &vkContext.instanceBufferMemory)) {
        return false;
    }

    vkContext.instanceCapacity = maxInstances;

    fprintf(stdout, "Created Vulkan instance buffer (capacity: %u)\n", maxInstances);

    return true;
}

static void CreateOrthoMatrix(float* o_matrix, float width, float height) {

    for (int i = 0; i < 16; i++)   
        o_matrix[i] = 0.0f;
    
    o_matrix[0] = 2.0f / width;
    o_matrix[5] = 2.0f / height;
    o_matrix[10] = 1.0f;
    o_matrix[12] = -1.0f;
    o_matrix[13] = -1.0f;
    o_matrix[15] = 1.0f;
}

static bool CreateDescriptorSetLayout() {
    const VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };

    if (vkCreateDescriptorSetLayout(vkContext.logicalDevice, &layoutInfo, NULL, &vkContext.descriptorSetLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        return false;
    }

    fprintf(stdout, "Created Vulkan descriptor set layout\n");

    return true;
}

static bool CreateUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    if (!CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vkContext.uniformBuffer, &vkContext.uniformBufferMemory)) {
        return false;
    }

    vkMapMemory(vkContext.logicalDevice, vkContext.uniformBufferMemory, 0, bufferSize, 0, &vkContext.uniformBufferMapped);

    UniformBufferObject ubo;
    CreateOrthoMatrix(ubo.ortho, (float)vkContext.swapchain.dimensions.width, (float)vkContext.swapchain.dimensions.height);
    memcpy(vkContext.uniformBufferMapped, &ubo, sizeof(ubo));

    fprintf(stdout, "Created Vulkan uniform buffer\n");

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

    vkDeviceWaitIdle(vkContext.logicalDevice);

    CleanupSwapchain();

    if (!CreateVkSwapchain(&vkContext.swapchain)) {
        return false;
    }

    if (!CreateFrameBuffers(&vkContext.swapchain)) {
        return false;
    }

    UniformBufferObject* ubo = (UniformBufferObject*)vkContext.uniformBufferMapped;
    CreateOrthoMatrix(ubo->ortho, (float)width, (float)height);

    fprintf(stdout, "Recreated Vulkan swapchain\n");

    return true;
}

static bool CreateDescriptorPoolAndSet() {

    const VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1
    };

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
        .maxSets = 1
    };

    if (vkCreateDescriptorPool(vkContext.logicalDevice, &poolInfo, NULL, &vkContext.descriptorPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor pool\n");
        return false;
    }

    const VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkContext.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vkContext.descriptorSetLayout
    };

    if (vkAllocateDescriptorSets(vkContext.logicalDevice, &allocateInfo, &vkContext.descriptorSet) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate descriptor set\n");
        return false;
    }

    const VkDescriptorBufferInfo bufferInfo = {
        .buffer = vkContext.uniformBuffer,
        .offset = 0,
        .range = sizeof(UniformBufferObject)
    };

    const VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vkContext.descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &bufferInfo
    };

    vkUpdateDescriptorSets(vkContext.logicalDevice, 1, &descriptorWrite, 0, NULL);

    fprintf(stdout, "Created Vulkan descriptor set\n");

    return true;
}

static void UpdateInstanceBuffer() {

    if (vkContext.rectangleCount == 0) {
        return;
    }

    VkDeviceSize dataSize = sizeof(RectangleInstance) * vkContext.rectangleCount;

    void* data;
    vkMapMemory(vkContext.logicalDevice, vkContext.instanceBufferMemory, 0, dataSize, 0, &data);
    memcpy(data, vkContext.rectangles, (size_t)dataSize);
    vkUnmapMemory(vkContext.logicalDevice, vkContext.instanceBufferMemory);
}

static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    VkContext* vkContext = (VkContext*)glfwGetWindowUserPointer(window);
    vkContext->frameBufferResized = true;
}

void VKK_Present() {
    vkWaitForFences(vkContext.logicalDevice, 1, &vkContext.inFlightFences[vkContext.currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult acquireResult = vkAcquireNextImageKHR(vkContext.logicalDevice, vkContext.swapchain.swapchainHandle, UINT64_MAX, vkContext.imageAvailableSemaphores[vkContext.currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    } else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        fprintf(stderr, "Failed to acquire swapchain image\n");
        return;
    }

    vkResetFences(vkContext.logicalDevice, 1, &vkContext.inFlightFences[vkContext.currentFrame]);

    UpdateInstanceBuffer();

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
        fprintf(stderr, "Failed to submit draw command buffer\n");
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
        fprintf(stderr, "Failed to present swapchain image\n");
    }

    vkContext.currentFrame = (vkContext.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    memset(vkContext.rectangles, 0, sizeof(vkContext.rectangles));
    vkContext.rectangleCount = 0;
}

void VKK_RenderRectangle(VKK_Rectangle rectangle) {

    RectangleInstance instance = {
        .offset[0] = rectangle.x,
        .offset[1] = rectangle.y,

        .scale[0] = rectangle.width,
        .scale[1] = rectangle.height,

        .color[0] = 1.0f,
        .color[1] = 0.0f,
        .color[2] = 0.0f,
    };

    vkContext.rectangles[vkContext.rectangleCount] = instance;
    vkContext.rectangleCount++;
}

bool VKK_Init(GLFWwindow* window) {

    glfwSetWindowUserPointer(window, &vkContext);
    glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);

    vkContext.window = window;

    glfwGetFramebufferSize(window, &vkContext.windowWidth, &vkContext.windowHeight);

    uint32_t instanceExtensionsAmount;
    const char** instanceExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionsAmount);

#ifdef DEBUG
    const char* layers[] = { 
        "VK_LAYER_KHRONOS_validation"
    };
    vkContext.layers = layers;
    vkContext.layersAmount = 1;
#else
    vkContext->layers = NULL;
    vkContext->layersAmount = 0;
#endif

    vkContext.extensionsAmount = instanceExtensionsAmount;
    vkContext.extensions = instanceExtensions;

    if (!CreateInstance()) {
        return false;
    }

    if (!CreateVkSurface()) {
        return false;
    }

    if (!GetPhysicalDevice()) {
        return false;
    }

    if (!CreateLogicalDevice()) {
        return false;
    }

    if (!CreateVkSwapchain(&vkContext.swapchain)) {
        return false;
    }

    if (!CreateRenderPass()) {
        return false;
    }

    if (!CreateDescriptorSetLayout()) {
        return false;
    }

    if (!CreateGraphicsPipeline()) {
        return false;
    }

    if (!CreateVertexBuffer()) {
        return false;
    }

    if (!CreateIndexBuffer()) {
        return false;
    }

    if (!CreateFrameBuffers(&vkContext.swapchain)) {
        return false;
    }

    if (!CreateCommandPool()) {
        return false;
    }

    if (!CreateCommandBuffers()) {
        return false;
    }

    if (!CreateSyncObjects()) {
        return false;
    }

    if (!CreateUniformBuffer()) {
        return false;
    }

    if (!CreateDescriptorPoolAndSet()) {
        return false;
    }

    vkContext.currentFrame = 0;

    int maxInstances = 100;
    vkContext.rectangles = malloc(sizeof(RectangleInstance) * maxInstances);

    vkContext.rectangleCount = 0;

    CreateInstanceBuffer(maxInstances);

    return true;
}

void VKK_End() {
    vkDeviceWaitIdle(vkContext.logicalDevice);
}