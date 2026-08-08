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

bool leftMousePressed;


typedef struct {
    float position[2];
    float uv[2];
} TexturedVertex;

static const TexturedVertex verticesLeft[] = {
    {{-1.0f, -1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f}, {0.0f, 1.0f}},
};

static const VKK_Vertex verticesRight[] = {
    {{-0.5f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{-1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
};

static const uint16_t indices[] = {
    0, 1, 2,
    0, 2, 3
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
        .enableValidationLayers = false,
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
        printf("[Device #%i] %s\n", i, devices[i].name);
    }

    VKK_PhysicalDeviceInfo deviceInfo;
    if (VKK_InitDevice(0, &deviceInfo) != VKK_SUCCESS) {
        fprintf(stderr, "Failed to initialize device\n");
        exit(1);
    }

    fprintf(stdout, "[Selected Device]: Name: %s, Device Id: %i, Device Type: %i\n", deviceInfo.name, deviceInfo.deviceID, deviceInfo.deviceType);
    
    VKK_PushConstantRange pushConstantRange = {
        .shaderStage = VKK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = sizeof(float) * 18
    };
    
    VKK_RendererConfig renderConfig = {
        .pushConstantRange = pushConstantRange,   
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
    
    VKK_Texture texture = VKK_CreateTexture("textures/katzi!.png");
    VKK_BindTexture(0, texture);

    VKK_Uniform timeUniform = VKK_CreateUniform(sizeof(float), VKK_SHADER_STAGE_VERTEX);
    VKK_BindUniform(1, timeUniform);
    
    VKK_VertexAttribute attributes[] = {
        { .location = 0, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(TexturedVertex, position)},
        { .location = 1, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(TexturedVertex, uv)},
    };

    VKK_PipelineDescription pipelineDescription = {
        .vertexShaderPath = "shader/compiled/vert.spv",
        .fragmentShaderPath = "shader/compiled/frag.spv",
        .attributes = attributes,
        .attributeCount = 2,
        .vertexStride = sizeof(TexturedVertex)
    };

    VKK_PipelineDescription solidPipelineDescription = {
        .vertexShaderPath = "shader/compiled/vert.spv",
        .fragmentShaderPath = "shader/compiled/fragSolid.spv",
        .attributes = attributes,
        .attributeCount = 2,
        .vertexStride = sizeof(VKK_Vertex)
    };

    VKK_Pipeline trianglePipeline = VKK_CreatePipeline(pipelineDescription);
    //VKK_Pipeline solidTrianglePipeline = VKK_CreatePipeline(solidPipelineDescription);

    VKK_Buffer vertexBuffer = VKK_CreateBuffer(sizeof(TexturedVertex) * 4, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(vertexBuffer, verticesLeft, sizeof(TexturedVertex) * 4, 0);

    VKK_Buffer solidVertexBuffer = VKK_CreateBuffer(sizeof(VKK_Vertex) * 3, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(solidVertexBuffer, verticesRight, sizeof(VKK_Vertex) * 3, 0);

    VKK_Buffer indexBuffer = VKK_CreateBuffer(sizeof(uint16_t) * 6, VKK_BUFFER_USAGE_INDEX);
    VKK_WriteBuffer(indexBuffer, indices, sizeof(uint16_t) * 6, 0);

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

        //VKK_Draw(solidTrianglePipeline, solidVertexBuffer, 3, indexBuffer, 3);
        VKK_Draw(trianglePipeline, vertexBuffer, 4, indexBuffer, 6);
    
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
    VKK_DestroyBuffer(solidVertexBuffer);
    VKK_DestroyBuffer(indexBuffer);

    VKK_DestroyPipeline(trianglePipeline);
    //VKK_DestroyPipeline(solidTrianglePipeline);

    VKK_DestroyTexture(texture);

    VKK_End();

    SDL_Quit();
}
