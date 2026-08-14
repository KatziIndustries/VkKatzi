#ifndef VKKATZI_H
#define VKKATZI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VKK_MAX_PHYSICAL_DEVICE_NAME_SIZE 256

typedef struct VKK_Buffer_T* VKK_Buffer;
typedef struct VKK_Uniform_T* VKK_Uniform;
typedef struct VKK_Pipeline_T* VKK_Pipeline;
typedef struct VKK_Instance_T* VKK_Instance;
typedef struct VKK_Surface_T* VKK_Surface;
typedef struct VKK_Texture_T* VKK_Texture;

typedef enum {
    VKK_VERTEX_FORMAT_FLOAT,
    VKK_VERTEX_FORMAT_FLOAT2,
    VKK_VERTEX_FORMAT_FLOAT3,
    VKK_VERTEX_FORMAT_FLOAT4
} VKK_VertexFormat;

typedef enum {
    VKK_SAMPLER_FILTER_NEAREST = 0,
    VKK_SAMPLER_FILTER_LINEAR = 1,
} VKK_SamplerFilter;

typedef enum {
    VKK_SAMPLER_ADDRESS_MODE_REPEAT = 0,
    VKK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
    VKK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
    VKK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
    VKK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
} VKK_SamplerAddressMode;

typedef struct {
    uint32_t location;
    VKK_VertexFormat format;
    uint32_t offset;
} VKK_VertexAttribute;

typedef struct {
    char* vertexShaderPath;
    char* fragmentShaderPath;

    VKK_VertexAttribute* attributes;
    uint32_t attributeCount;
    uint32_t vertexStride;

    uint32_t instanceStride;
    VKK_VertexAttribute* instanceAttributes;
    uint32_t instanceAttributesCount;
} VKK_PipelineDescription;

typedef enum {
    VKK_BUFFER_USAGE_VERTEX,
    VKK_BUFFER_USAGE_INDEX,
    VKK_BUFFER_USAGE_UNIFORM,
    VKK_BUFFER_USAGE_STORAGE,
    VKK_BUFFER_USAGE_STAGING,
} VKK_BufferUsage;

typedef enum {
    VKK_PRESENT_MODE_IMMEDIATE = 0,
    VKK_PRESENT_MODE_MAILBOX = 1,
    VKK_PRESENT_MODE_FIFO = 2,
    VKK_PRESENT_MODE_FIFO_RELAXED = 3,
    VKK_PRESENT_MODE_SHARED_DEMAND_REFRESH = 1000111000,
    VKK_PRESENT_MODE_SHARED_CONTINOUS_REFRESH = 1000111001,
    VKK_PRESENT_MODE_FIFO_LATEST_READY = 1000361000
} VKK_PresentMode;

typedef enum {
    VKK_SHADER_STAGE_VERTEX,
    VKK_SHADER_STAGE_FRAGMENT,
    VKK_SHADER_STAGE_ALL,
} VKK_ShaderStage;

typedef enum {
    VKK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
    VKK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
    VKK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VKK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
    VKK_PHYSICAL_DEVICE_TYPE_CPU = 4,
    VKK_PHYSICAL_DEVICE_TYPE_MAX_ENUM = 0x7FFFFFFF
} VKK_PhysicalDeviceType;

typedef struct {
    VKK_PresentMode presentMode;
    uint32_t imageBufferSize;
    bool enableValidationLayers;
    bool logWarnings;

    const char** requiredExtensions;
    uint32_t requiredExtensionsCount;
} VKK_Config;

typedef struct {
    uint32_t maxImageDimension1D;
    uint32_t maxImageDimension2D;
    uint32_t maxImageDimension3D;
    uint32_t maxImageDimensionCube;
    uint32_t maxImageArrayLayers;
    uint32_t maxTexelBufferElements;
    uint32_t maxUniformBufferRange;
    uint32_t maxStorageBufferRange;
    uint32_t maxPushConstantsSize;
    uint32_t maxMemoryAllocationCount;
    uint32_t maxSamplerAllocationCount;
    uint64_t bufferImageGranularity;
    uint64_t sparseAddressSpaceSize;
    uint32_t maxBoundDescriptorSets;
    uint32_t maxPerStageDescriptorSamplers;
    uint32_t maxPerStageDescriptorUniformBuffers;
    uint32_t maxPerStageDescriptorStorageBuffers;
    uint32_t maxPerStageDescriptorSampledImages;
    uint32_t maxPerStageDescriptorStorageImages;
    uint32_t maxPerStageDescriptorInputAttachments;
    uint32_t maxPerStageResources;
    uint32_t maxDescriptorSetSamplers;
    uint32_t maxDescriptorSetUniformBuffers;
    uint32_t maxDescriptorSetUniformBuffersDynamic;
    uint32_t maxDescriptorSetStorageBuffers;
    uint32_t maxDescriptorSetStorageBuffersDynamic;
    uint32_t maxDescriptorSetSampledImages;
    uint32_t maxDescriptorSetStorageImages;
    uint32_t maxDescriptorSetInputAttachments;
    uint32_t maxVertexInputAttributes;
    uint32_t maxVertexInputBindings;
    uint32_t maxVertexInputAttributeOffset;
    uint32_t maxVertexInputBindingStride;
    uint32_t maxVertexOutputComponents;
    uint32_t maxTessellationGenerationLevel;
    uint32_t maxTessellationPatchSize;
    uint32_t maxTessellationControlPerVertexInputComponents;
    uint32_t maxTessellationControlPerVertexOutputComponents;
    uint32_t maxTessellationControlPerPatchOutputComponents;
    uint32_t maxTessellationControlTotalOutputComponents;
    uint32_t maxTessellationEvaluationInputComponents;
    uint32_t maxTessellationEvaluationOutputComponents;
    uint32_t maxGeometryShaderInvocations;
    uint32_t maxGeometryInputComponents;
    uint32_t maxGeometryOutputComponents;
    uint32_t maxGeometryOutputVertices;
    uint32_t maxGeometryTotalOutputComponents;
    uint32_t maxFragmentInputComponents;
    uint32_t maxFragmentOutputAttachments;
    uint32_t maxFragmentDualSrcAttachments;
    uint32_t maxFragmentCombinedOutputResources;
    uint32_t maxComputeSharedMemorySize;
    uint32_t maxComputeWorkGroupCount[3];
    uint32_t maxComputeWorkGroupInvocations;
    uint32_t maxComputeWorkGroupSize[3];
    uint32_t subPixelPrecisionBits;
    uint32_t subTexelPrecisionBits;
    uint32_t mipmapPrecisionBits;
    uint32_t maxDrawIndexedIndexValue;
    uint32_t maxDrawIndirectCount;
    float maxSamplerLodBias;
    float maxSamplerAnisotropy;
    uint32_t maxViewports;
    uint32_t maxViewportDimensions[2];
    float viewportBoundsRange[2];
    uint32_t viewportSubPixelBits;
    size_t minMemoryMapAlignment;
    uint64_t minTexelBufferOffsetAlignment;
    uint64_t minUniformBufferOffsetAlignment;
    uint64_t minStorageBufferOffsetAlignment;
    int32_t minTexelOffset;
    uint32_t maxTexelOffset;
    int32_t minTexelGatherOffset;
    uint32_t maxTexelGatherOffset;
    float minInterpolationOffset;
    float maxInterpolationOffset;
    uint32_t subPixelInterpolationOffsetBits;
    uint32_t maxFramebufferWidth;
    uint32_t maxFramebufferHeight;
    uint32_t maxFramebufferLayers;
    uint32_t framebufferColorSampleCounts;
    uint32_t framebufferDepthSampleCounts;
    uint32_t framebufferStencilSampleCounts;
    uint32_t framebufferNoAttachmentsSampleCounts;
    uint32_t maxColorAttachments;
    uint32_t sampledImageColorSampleCounts;
    uint32_t sampledImageIntegerSampleCounts;
    uint32_t sampledImageDepthSampleCounts;
    uint32_t sampledImageStencilSampleCounts;
    uint32_t storageImageSampleCounts;
    uint32_t maxSampleMaskWords;
    bool timestampComputeAndGraphics;
    float timestampPeriod;
    uint32_t maxClipDistances;
    uint32_t maxCullDistances;
    uint32_t maxCombinedClipAndCullDistances;
    uint32_t discreteQueuePriorities;
    float pointSizeRange[2];
    float lineWidthRange[2];
    float pointSizeGranularity;
    float lineWidthGranularity;
    bool strictLines;
    bool standardSampleLocations;
    uint64_t optimalBufferCopyOffsetAlignment;
    uint64_t optimalBufferCopyRowPitchAlignment;
    uint64_t nonCoherentAtomSize; 
} VKK_PhysicalDeviceLimits;

typedef struct {
    bool residencyStandard2DBlockShape;
    bool residencyStandard2DMultisampleBlockShape;
    bool residencyStandard3DBlockShape;
    bool residencyAlignedMipSize;
    bool residencyNonResidentStrict;
} VKK_PhysicalDeviceSparseProperties;

typedef struct {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    VKK_PhysicalDeviceType deviceType;
    char deviceName[VKK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    VKK_PhysicalDeviceLimits limits;
    VKK_PhysicalDeviceSparseProperties sparseProperties;
} VKK_PhysicalDeviceProperties;

typedef struct {
    bool robustBufferAccess;
    bool fullDrawIndexUint32;
    bool imageCubeArray;
    bool independentBlend;
    bool geometryShader;
    bool tessellationShader;
    bool sampleRateShading;
    bool dualSrcBlend;
    bool logicOp;
    bool multiDrawIndirect;
    bool drawIndirectFirstInstance;
    bool depthClamp;
    bool depthBiasClamp;
    bool fillModeNonSolid;
    bool depthBounds;
    bool wideLines;
    bool largePoints;
    bool alphaToOne;
    bool multiViewport;
    bool samplerAnisotropy;
    bool textureCompressionETC2;
    bool textureCompressionASTC_LDR;
    bool textureCompressionBC;
    bool occlusionQueryPrecise;
    bool pipelineStatisticsQuery;
    bool vertexPipelineStoresAndAtomics;
    bool fragmentStoresAndAtomics;
    bool shaderTessellationAndGeometryPointSize;
    bool shaderImageGatherExtended;
    bool shaderStorageImageExtendedFormats;
    bool shaderStorageImageMultisample;
    bool shaderStorageImageReadWithoutFormat;
    bool shaderStorageImageWriteWithoutFormat;
    bool shaderUniformBufferArrayDynamicIndexing;
    bool shaderSampledImageArrayDynamicIndexing;
    bool shaderStorageBufferArrayDynamicIndexing;
    bool shaderStorageImageArrayDynamicIndexing;
    bool shaderClipDistance;
    bool shaderCullDistance;
    bool shaderFloat64;
    bool shaderInt64;
    bool shaderInt16;
    bool shaderResourceResidency;
    bool shaderResourceMinLod;
    bool sparseBinding;
    bool sparseResidencyBuffer;
    bool sparseResidencyImage2D;
    bool sparseResidencyImage3D;
    bool sparseResidency2Samples;
    bool sparseResidency4Samples;
    bool sparseResidency8Samples;
    bool sparseResidency16Samples;
    bool sparseResidencyAliased;
    bool variableMultisampleRate;
    bool inheritedQueries;
} VKK_PhysicalDeviceFeatures;

typedef struct {
    VKK_PhysicalDeviceProperties properties;
    VKK_PhysicalDeviceFeatures features;
} VKK_PhysicalDeviceInfo;

typedef struct {
    VKK_ShaderStage shaderStage;
    uint32_t offset;
    uint32_t size;
} VKK_PushConstantRange;

typedef struct {
    VKK_PushConstantRange pushConstantRange;
    uint32_t maxDescriptorSets;
} VKK_RendererConfig;

typedef struct {
    uint32_t versionMajor;
    uint32_t versionMinor;
    uint32_t versionPatch;
    VKK_Instance instance;
} VKK_InstanceInfo;

typedef enum {
    VKK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
    VKK_BORDER_COLOR_INT_TRANSPARENT_BLACK = 1,
    VKK_BORDER_COLOR_FLOAT_OPAQUE_BLACK = 2,
    VKK_BORDER_COLOR_INT_OPAQUE_BLACK = 3,
    VKK_BORDER_COLOR_FLOAT_OPAQUE_WHITE = 4,
    VKK_BORDER_COLOR_INT_OPAQUE_WHITE = 5,
    VKK_BORDER_COLOR_FLOAT_CUSTOM_EXT = 1000287003,
    VKK_BORDER_COLOR_INT_CUSTOM_EXT = 1000287004,
} VKK_SamplerBorderColor;

typedef enum {
    VKK_SUCCESS = 0,
    VKK_ERROR_INSTANCE_CREATION_FAILED,
    VKK_ERROR_SURFACE_CREATION_FAILED,
    VKK_ERROR_NO_SUITABLE_DEVICE,
    VKK_ERROR_INVALID_DEVICE_INDEX,
    VKK_ERROR_DEVICE_CREATION_FAILED,
    VKK_ERROR_SWAPCHAIN_CREATION_FAILED,
    VKK_ERROR_RENDER_PASS_CREATION_FAILED,
    VKK_ERROR_DESCRIPTOR_SET_LAYOUT_CREATION_FAILED,
    VKK_ERROR_FRAMEBUFFER_CREATION_FAILED,
    VKK_ERROR_COMMAND_POOL_CREATION_FAILED,
    VKK_ERROR_COMMAND_BUFFER_CREATION_FAILED,
    VKK_ERROR_SYNC_OBJECTS_CREATION_FAILED,
    VKK_ERROR_DESCRIPTOR_POOL_CREATION_FAILED,
    VKK_ERROR_WRONG_EXECUTION_ORDER,
} VKK_Result;

typedef struct {
    float r;
    float g;
    float b;
    float a;
} VKK_Color;

typedef struct {
    VKK_SamplerFilter filter;
    VKK_SamplerAddressMode addressMode;
    VKK_SamplerBorderColor borderColor;
} VKK_SamplerInfo;

typedef enum {
    VKK_DESCRIPTOR_TYPE_SAMPLER = 0,
    VKK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
    VKK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
    VKK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
    VKK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
    VKK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
    VKK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
    VKK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
    VKK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
    VKK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
    VKK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    VKK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK = 1000138000,
    VKK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 1000150000,
    VKK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV = 1000165000,
    VKK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM = 1000440000,
    VKK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM = 1000440001,
    VKK_DESCRIPTOR_TYPE_TENSOR_ARM = 1000460000,
    VKK_DESCRIPTOR_TYPE_MUTABLE_EXT = 1000351000,
    VKK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV = 1000570000,
    VKK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT = VKK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
    VKK_DESCRIPTOR_TYPE_MUTABLE_VALVE = VKK_DESCRIPTOR_TYPE_MUTABLE_EXT,
} VKK_DescriptorType;

typedef struct {
    uint32_t binding;
    VKK_DescriptorType descriptorType;
    VKK_ShaderStage shaderStage;
} VKK_DescriptorSetLayoutBinding;

VKK_Result VKK_InitInstance(VKK_Config config, VKK_InstanceInfo* o_instanceInfo);

uint32_t VKK_EnumeratePhysicalDevices(VKK_PhysicalDeviceInfo* o_devices, uint32_t maxDevices);
VKK_Result VKK_InitDevice(uint32_t deviceIndex, VKK_PhysicalDeviceInfo* o_deviceInfo);

VKK_Result VKK_InitRenderer(VKK_RendererConfig rendererConfig);
void VKK_End(void);

void VKK_SetFramebufferSize(uint32_t width, uint32_t height);

void VKK_Present(VKK_Color clearColor);

VKK_Buffer VKK_CreateBuffer(size_t size, VKK_BufferUsage usage);
void VKK_DestroyBuffer(VKK_Buffer buffer);
void VKK_WriteBuffer(VKK_Buffer buffer, const void* data, size_t size, size_t offset);

VKK_Uniform VKK_CreateUniform(size_t size, VKK_ShaderStage shaderStage);
void VKK_BindUniform(uint32_t binding, VKK_Uniform uniform);
void VKK_WriteUniform(VKK_Uniform uniform, const void* data, size_t size, size_t offset);
void VKK_DestroyUniform(VKK_Uniform uniform);

void VKK_Draw(VKK_Pipeline pipeline, VKK_Buffer vertexBuffer, uint32_t vertexCount, VKK_Buffer indexBuffer, uint32_t indexCount);
void VKK_DrawInstanced(VKK_Pipeline pipeline, VKK_Buffer vertexBuffer, uint32_t vertexCount, VKK_Buffer indexBuffer, uint32_t indexCount, VKK_Buffer instanceBuffer, uint32_t instanceCount);

void VKK_SetPushConstantData(void* data);

VKK_Pipeline VKK_CreatePipeline(VKK_PipelineDescription desc);
void VKK_DestroyPipeline(VKK_Pipeline pipeline);

VKK_Texture VKK_CreateTexture(const char* path);
VKK_Texture VKK_CreateTextureFromPixels(const void* pixels, uint32_t width, uint32_t height);
void VKK_DestroyTexture(VKK_Texture texture);
void VKK_SetTextureSampler(VKK_Texture texture, VKK_SamplerInfo samplerInfo);

VKK_Result VKK_CreateDescriptorSetLayout(VKK_DescriptorSetLayoutBinding* bindings, uint32_t bindingsCount);

void VKK_BindTexture(uint32_t binding, VKK_Texture texture);

void VKK_SetSurface(VKK_Surface surface, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

#endif
