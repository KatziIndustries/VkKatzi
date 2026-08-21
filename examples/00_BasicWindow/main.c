#include <stdio.h>

#include "../../include/vkkatzi.h"
#include "../../include/vkkatzi_SDL3.h"

// This is the minimal stuff you have to do to clear the screen with a color

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

    // Can stay empty for now
    VKK_RendererConfig rendererConfig = {};

    if (VKK_InitRenderer(rendererConfig) != VKK_SUCCESS) {
        printf("Failed to initialize renderer\n");
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // This is supposed to be Cornflowerblue but it doesn't work for some reason
        VKK_Color clearColor = {
            .r = 0.392,
            .g = 0.584,
            .b = 0.929,
            .a = 1.0   
        };
        
        VKK_Present(clearColor);
    }

    VKK_End();
    SDL_Quit();

    return 0;
}