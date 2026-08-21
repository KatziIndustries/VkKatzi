# VkKatzi

VkKatzi is a simple general purpose layer of abstraction on top of Vulkan.

currently VkKatzi only supports SDL3 for windowing but other options will be added in the future probably.

# Building from source

## Linux

Prerequisites:
- gcc
- vulkan sdk
- (optional) SDL3

```
git clone https://github.com/KatziIndustries/VkKatzi.git
cd VkKatzi
make install
```

The final library will be located in the build directory and also automatically copied to /usr/lib.
If you don't want the library to be copied just run make instead of make install

## Windows

Install linux


