@echo off
setlocal

if not exist build mkdir build

echo [1/3] Building vkkatzi.dll...
cl /LD vulkan.c logger.c ^
    /Fe:build\vkkatzi.dll ^
    /link vulkan-1.lib

if errorlevel 1 exit /b 1

echo [2/3] Building vkkatzi_glfw.dll...
cl /LD glfw.c ^
    /Fe:build\vkkatzi_glfw.dll ^
    /link build\vkkatzi.lib glfw3.lib

if errorlevel 1 exit /b 1

echo [3/3] Building vkkatzi_sdl3.dll...
cl /LD sdl.c ^
    /Fe:build\vkkatzi_sdl3.dll ^
    /link build\vkkatzi.lib SDL3.lib

if errorlevel 1 exit /b 1

echo.
echo Build successful!
echo.

endlocal
