mkdir -p build

gcc -shared -fPIC vulkan.c -o build/libvkkatzi.so -lvulkan
gcc -shared -fPIC glfw.c -o build/libvkkatzi_glfw.so -Lbuild -lvkkatzi
gcc -shared -fPIC sdl.c -o build/libvkkatzi_sdl3.so -Lbuild -lvkkatzi