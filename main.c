#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "include/shared.h"
#include "include/vkkatzi.h"
#include "include/vkkatzi_sdl.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

uint32_t windowWidth;
uint32_t windowHeight;

typedef struct {
    float position[2];
    float uv[2];
} TexturedVertex;

typedef struct {
    float position[2];
} Vertex;

typedef struct {
    float offset[2];
    float color[3];
} InstanceData;

static const TexturedVertex vertices[] = {
    {{-0.5f, -0.5f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f}, {0.0f, 1.0f}},
};

static const uint32_t indices[] = {
    0, 1, 2,
    0, 2, 3
};

static const Vertex instanceVertices[] = {
    { { 0.0f, -0.05f } },
    { { 0.05f, 0.05f } },
    { { -0.05f, 0.05f } }
};

static const uint32_t triangleIndices[] = {
    0, 1, 2
};

static const InstanceData instances[] = {
    { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
    { {  0.0f,  0.0f }, { 0.0f, 1.0f, 0.0f } }, 
    { {  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
};

float* mergeArrays(float arr1[], int n1, float arr2[], int n2) {
  	float *res = (float*)malloc(sizeof(float) * (n1 + n2));
    memcpy(res, arr1, n1 * sizeof(float));
    memcpy(res + n1, arr2, n2 * sizeof(float));
  	return res;
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

int main() {

#ifdef RAPHI
    if (Programm.start) {
        Programm = gut;
    } else {
        Programm = schlecht;
    }
#endif

    if (!SDL_Init(SDL_INIT_VIDEO)){
        fprintf(stderr, "Failed to initialize SDL: %s", SDL_GetError());
        exit(1);
    }

    SDL_Window* window = SDL_CreateWindow("Katzi lel", 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    uint32_t requiredExtensionsCount = 0;
    const char* const* requiredExtensions = SDL_Vulkan_GetInstanceExtensions(&requiredExtensionsCount);

    VKK_Config config = {
        .presentMode = VKK_PRESENT_MODE_MAILBOX,
        .imageBufferSize = 3,
        .enableValidationLayers = true,
        .logWarnings = true,
        .requiredExtensions = (const char**)requiredExtensions,
        .requiredExtensionsCount = requiredExtensionsCount
    };

    VKK_InstanceInfo instanceInfo;
    if (VKK_InitInstance(config, &instanceInfo) != VKK_SUCCESS) {
        fprintf(stderr, "Failed to initialize Vulkan context\n");
        exit(1);
    }

    VKK_Surface surface;
    VKK_CreateSurfaceSDL(window, instanceInfo.instance, &surface);

    int surfaceWidth;
    int surfaceHeight;
    SDL_GetWindowSize(window, &surfaceWidth, &surfaceHeight);

    VKK_SetSurface(surface, surfaceWidth, surfaceHeight);

    printf("[Vulkan] Version: %i.%i.%i\n", instanceInfo.versionMajor, instanceInfo.versionMinor, instanceInfo.versionPatch);

    VKK_PhysicalDeviceInfo devices[8];
    uint32_t count = VKK_EnumeratePhysicalDevices(devices, 8);

    for (uint32_t i = 0; i < count; i++) {
        printf("[Device #%i] %s\n", i, devices[i].properties.deviceName);
    }

    VKK_PhysicalDeviceInfo deviceInfo;
    if (VKK_InitDevice(0, &deviceInfo) != VKK_SUCCESS) {
        fprintf(stderr, "Failed to initialize device\n");
        exit(1);
    }

    fprintf(stdout, "[Selected Device]: Name: %s, Device Id: %i, Device Type: %i\n", deviceInfo.properties.deviceName, deviceInfo.properties.deviceID, deviceInfo.properties.deviceType);
    printf("[Device Features]: Anisotropy: %d, Max Anisotropy: %f\n", deviceInfo.features.samplerAnisotropy, deviceInfo.properties.limits.maxSamplerAnisotropy);
    
    VKK_PushConstantRange pushConstantRange = {
        .shaderStage = VKK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = sizeof(float) * 18
    };
    
    VKK_RendererConfig renderConfig = {
        .pushConstantRange = pushConstantRange
    };

    VKK_DescriptorSetLayoutBinding bindings[] = {
        { .binding = 0, .descriptorType = VKK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .shaderStage = VKK_SHADER_STAGE_FRAGMENT },
        { .binding = 1, .descriptorType = VKK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .shaderStage = VKK_SHADER_STAGE_ALL },
    };

    if (VKK_CreateDescriptorSetLayout(bindings, 2) != VKK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor set layout");
        exit(1);
    }

    if (VKK_InitRenderer(renderConfig) != VKK_SUCCESS) {
        fprintf(stderr, "Failed to initialize pipeline\n");
        exit(1);
    }

    VKK_Uniform timeUniform = VKK_CreateUniform(sizeof(float), VKK_SHADER_STAGE_VERTEX);
    VKK_BindUniform(1, timeUniform);

    VKK_Texture texture = VKK_CreateTexture("textures/github.jpg", VKK_FORMAT_R8G8B8A8_SRGB);
    VKK_Texture texture2 = VKK_CreateTexture("textures/katzi!.png", VKK_FORMAT_R8G8B8A8_SRGB);

    uint32_t textureWidth, textureHeight;
    VKK_GetTextureSize(texture2, &textureWidth, &textureHeight);

    printf("Width: %d, Height: %d\n", textureWidth, textureHeight);

    VKK_SamplerInfo textureSampler = {
        .addressMode = VKK_SAMPLER_ADDRESS_MODE_REPEAT,
        .borderColor = VKK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        .filter = VKK_SAMPLER_FILTER_LINEAR,
        .enableAnisotropy = true,
        .maxAnisotropy = 16.0f
    };

    VKK_SetTextureSampler(texture, textureSampler);
    VKK_SetTextureSampler(texture2, textureSampler);
    
    VKK_BindTexture(0, texture2);
    
    VKK_Pipeline trianglePipeline;
    {
        VKK_VertexAttribute attributes[] = {
            { .location = 0, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(TexturedVertex, position)},
            { .location = 1, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(TexturedVertex, uv)},
        };
    
        VKK_PipelineDescription pipelineDescription = {
            .vertexShaderPath = "shader/compiled/vert.spv",
            .fragmentShaderPath = "shader/compiled/frag.spv",
            .attributes = attributes,
            .attributeCount = 2,
            .vertexStride = sizeof(TexturedVertex),
            .rasterizer = {
                .cullMode = VKK_CULL_MODE_BACK,
                .frontFace = VKK_FRONT_FACE_CLOCKWISE,
                .lineWidth = 1.0f,
                .polygonMode = VKK_POLYGON_MODE_FILL
            }
        };

        trianglePipeline = VKK_CreatePipeline(pipelineDescription);
    }

    VKK_Pipeline instancedPipeline;
    {
        VKK_VertexAttribute vertexAttributes[] = {
            { .location = 0, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(Vertex, position) }
        };

        VKK_VertexAttribute instanceAttributes[] = {
            { .location = 1, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(InstanceData, offset) },
            { .location = 2, .format = VKK_VERTEX_FORMAT_FLOAT3, .offset = offsetof(InstanceData, color) },
        };

        VKK_PipelineDescription desc = {
            .vertexShaderPath = "shader/compiled/instancedVert.spv",
            .fragmentShaderPath = "shader/compiled/instancedFrag.spv",
            .vertexStride = sizeof(Vertex),
            .attributes = vertexAttributes,
            .attributeCount = 1,
            .instanceStride = sizeof(InstanceData),
            .instanceAttributes = instanceAttributes,
            .instanceAttributesCount = 2,
            .rasterizer = {
                .cullMode = VKK_CULL_MODE_BACK,
                .frontFace = VKK_FRONT_FACE_CLOCKWISE,
                .lineWidth = 1.0f,
                .polygonMode = VKK_POLYGON_MODE_LINE
            }
        };

        instancedPipeline = VKK_CreatePipeline(desc);
    }


    VKK_Buffer vertexBuffer = VKK_CreateBuffer(sizeof(TexturedVertex) * 4, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(vertexBuffer, vertices, sizeof(TexturedVertex) * 4, 0);

    VKK_Buffer indexBuffer = VKK_CreateBuffer(sizeof(uint32_t) * 6, VKK_BUFFER_USAGE_INDEX);
    VKK_WriteBuffer(indexBuffer, indices, sizeof(uint32_t) * 6, 0);


    VKK_Buffer instanceVertexBuffer = VKK_CreateBuffer(sizeof(Vertex) * 3, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(instanceVertexBuffer, instanceVertices, sizeof(Vertex) * 3, 0);

    VKK_Buffer instanceIndexBuffer = VKK_CreateBuffer(sizeof(uint32_t) * 3, VKK_BUFFER_USAGE_INDEX);
    VKK_WriteBuffer(instanceIndexBuffer, triangleIndices, sizeof(uint32_t) * 3, 0);

    VKK_Buffer instanceBuffer = VKK_CreateBuffer(sizeof(InstanceData) * 3, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(instanceBuffer, instances, sizeof(InstanceData) * 3, 0);

    float elapsedTime = 0;
    float elapsedTotal = 0;
    
    Uint64 lastFrameTime = SDL_GetPerformanceCounter();

    int lastWindowWidth = surfaceWidth;
    int lastWindowHeight = surfaceHeight;

    bool running = true;

    while (running) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        Uint64 currentTime = SDL_GetPerformanceCounter();
        double deltaTime = (double)(currentTime - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
        lastFrameTime = currentTime;
    
        elapsedTime += deltaTime;
        elapsedTotal += deltaTime;
    
        if (elapsedTime > 1) {
            fprintf(stdout, "Frametime: %lf, FPS: %lf\n", deltaTime, 1.0f / deltaTime);
            fflush(stdout);
            elapsedTime = 0;
        }
    
        
        float mousePosF[2];
        SDL_GetMouseState(&(mousePosF[0]), &(mousePosF[1]));
    
        float matrix[16];
        CreateOrthoMatrix(matrix, windowWidth, windowHeight);
    
        float* pushData = mergeArrays(matrix, 16, mousePosF, 2);

        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        if (windowWidth != lastWindowWidth || windowHeight != lastWindowHeight) {
            VKK_SetFramebufferSize(windowWidth, windowHeight);

            lastWindowWidth = windowWidth;
            lastWindowHeight = windowHeight;
        }
        
        VKK_SetPushConstantData(pushData);

        VKK_WriteUniform(timeUniform, &elapsedTime, sizeof(float), 0);

        VKK_Draw(trianglePipeline, vertexBuffer, indexBuffer, 6);
        VKK_DrawInstanced(instancedPipeline, instanceVertexBuffer, instanceIndexBuffer, 3, instanceBuffer, 3);
    
        VKK_Color clearColor = {
            .r = 0.0f,
            .g = 0.0f,
            .b = 0.0f,
            .a = 1.0f,
        };
    
        VKK_Present(clearColor);
    
        free(pushData);
    }

    VKK_DestroyBuffer(vertexBuffer);
    VKK_DestroyBuffer(indexBuffer);

    VKK_DestroyBuffer(instanceVertexBuffer);
    VKK_DestroyBuffer(instanceIndexBuffer);
    VKK_DestroyBuffer(instanceBuffer);

    VKK_DestroyPipeline(trianglePipeline);
    VKK_DestroyPipeline(instancedPipeline);

    VKK_DestroyTexture(texture);
    VKK_DestroyTexture(texture2);

    VKK_DestroyUniform(timeUniform);

    VKK_End();

    SDL_Quit();
}
