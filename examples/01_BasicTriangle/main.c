#include <stdio.h>

#include "../../include/vkkatzi.h"
#include "../../include/vkkatzi_SDL3.h"

typedef struct {
    float pos[2];
    float color[4];
} Vertex;

// Vertices and indices for our triangle
static const Vertex vertices[] = {
    { {  0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    { {  1.0f,  1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
    { { -1.0f,  1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
};

static const uint32_t indices[] = {
    0, 1, 2,
    0, 2, 3
};

int main() {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Failed to load SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Basic Window example", 800, 600, SDL_WINDOW_VULKAN);

    uint32_t requiredExtensionsCount = 0;
    const char* const* requiredExtensions = SDL_Vulkan_GetInstanceExtensions(&requiredExtensionsCount);

    VKK_Config config = {
        .presentMode = VKK_PRESENT_MODE_FIFO,
        .imageBufferSize = 3,
        .enableValidationLayers = true,
        .logWarnings = true,
        .requiredExtensions = (const char**)requiredExtensions,
        .requiredExtensionsCount = requiredExtensionsCount
    };

    VKK_InstanceInfo instanceInfo;
    if (VKK_InitInstance(config, &instanceInfo) != VKK_SUCCESS) {
        printf("Failed to initialize instance\n");
        return 1;
    }

    VKK_Surface surface;
    VKK_CreateSurfaceSDL(window, instanceInfo.instance, &surface);

    VKK_SetSurface(surface, 800, 600);

    VKK_PhysicalDeviceInfo devices[8];
    uint32_t deviceCount = VKK_EnumeratePhysicalDevices(devices, 8);

    // prints all found gpus and their index
    for (uint32_t i = 0; i < deviceCount; i++) {
        printf("[Device #%i] Name: %s\n", i, devices[i].properties.deviceName);
    }

    VKK_PhysicalDeviceInfo deviceInfo;
    // 0 can be replaced with the index of your preferred gpu
    if (VKK_InitDevice(0, &deviceInfo) != VKK_SUCCESS) {
        printf("Failed to initialize device\n");
        return 1;
    }

    // Doesn't do anything but we still have to call it for some reason
    if (VKK_CreateDescriptorSetLayout(NULL, 0) != VKK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        return 1;
    }

    // We will have to create a pipeline to render our triangle to the screen
    VKK_Pipeline pipeline;
    {
        // You don't have to put this into a scope but I just like to do it

        VKK_VertexAttribute attributes[] = {
            { .location = 0, .format = VKK_VERTEX_FORMAT_FLOAT2, .offset = offsetof(Vertex, pos) },
            { .location = 1, .format = VKK_VERTEX_FORMAT_FLOAT4, .offset = offsetof(Vertex, color) },
        };

        VKK_PipelineDescription desc = {
            .vertexShaderPath = "shader/compiled/vertex.vert.spv",
            .fragmentShaderPath = "shader/compiled/fragment.frag.spv",
            .attributes = attributes,
            .attributeCount = 2,
            .vertexStride = sizeof(Vertex),
            .rasterizer = {
                .cullMode = VKK_CULL_MODE_BACK,
                .frontFace = VKK_FRONT_FACE_CLOCKWISE,
                .polygonMode = VKK_POLYGON_MODE_FILL
            }
        };

        pipeline = VKK_CreatePipeline(desc);
    }

    // Can stay empty for now
    VKK_RendererConfig rendererConfig = {};

    if (VKK_InitRenderer(rendererConfig) != VKK_SUCCESS) {
        printf("Failed to initialize renderer\n");
        return 1;
    }

    // Create the vertex buffer and write the vertices into it
    VKK_Buffer vertexBuffer = VKK_CreateBuffer(sizeof(Vertex) * 3, VKK_BUFFER_USAGE_VERTEX);
    VKK_WriteBuffer(vertexBuffer, vertices, sizeof(Vertex) * 3, 0);

    // For the index buffer aswell
    VKK_Buffer indexBuffer = VKK_CreateBuffer(sizeof(uint32_t) * 6, VKK_BUFFER_USAGE_INDEX);
    VKK_WriteBuffer(indexBuffer, indices, sizeof(uint32_t) * 6, 0);



    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // Call a draw call to draw to draw our triangle to the screen
        VKK_Draw(pipeline, vertexBuffer, indexBuffer, 6);

        // This is supposed to be Cornflowerblue but it doesn't work for some reason
        VKK_Color clearColor = {
            .r = 0.392,
            .g = 0.584,
            .b = 0.929,
            .a = 1.0   
        };
        
        VKK_Present(clearColor);
    }

    VKK_DestroyBuffer(vertexBuffer);
    VKK_DestroyBuffer(indexBuffer);

    VKK_DestroyPipeline(pipeline);

    VKK_End();
    SDL_Quit();

    return 0;
}