
#pragma once

#include <algorithm>
#include "vulkan/vulkan_core.h"
//#include <vulkan/vulkan.hpp>  // A modern C++ API for Vulkan. Beware 14K lines of code
   
// Imgui
// This can be defined for IMGUI, but only after some of the Vulkan initialization is complete.
#undef GUI

// Define this to read the San_Miguel model instead of the default living room model.
//#define SAN_MIGUEL

#ifdef GUI
#include "backends/imgui_impl_glfw.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#endif

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "shaders/shared_structs.h"

#include "wrap_misc.h"
#include "wrap_buffer.h"
#include "wrap_descriptor.h"
#include "wrap_acceleration.h"

#define GLM_FORCE_CTOR_INIT  // May be needed by recent versions of GLM;
                             // May also need mat4(1.0) instead of mat4();
// Make rotations use radians not degrees.
#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>

// The OBJ model: Vulkan buffers of object data
struct ObjData
{
    uint32_t     nbIndices{0};
    uint32_t     nbVertices{0};
    BufferWrap vertexBuffer;    // Buffer of vertices 
    BufferWrap indexBuffer;     // Buffer of triangle indices
    BufferWrap matColorBuffer;  // Buffer of materials
    BufferWrap matIndexBuffer;  // Buffer of each triangle's material index
};

#define NAME(handle, objType, name)  { \
        const VkDebugUtilsObjectNameInfoEXT imageNameInfo = {\
            VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, \
            NULL, \
            objType, \
            (uint64_t)handle, name\
        }; \
        vkSetDebugUtilsObjectNameEXT(m_device, &imageNameInfo); }


// Pair each instance with its instance transform
struct ObjInst
{
    glm::mat4 transform;    // Matrix of the instance
    uint32_t  objIndex;     // Model index
};

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

class App;

class VkApp
{
public:
    std::vector<const char*> reqInstanceExtensions = {  // GLFW will add to this
        "VK_EXT_debug_utils"                            // Can help debug validation errors
    };
    
    std::vector<const char*> reqInstanceLayers = {
        //"VK_LAYER_LUNARG_api_dump",  // Useful, but prefer to add (or not) at runtime
        "VK_LAYER_KHRONOS_validation"  // ALWAYS!!
    };
    
    std::vector<const char*> reqDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,		 // Presentation engine; draws to screen
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,	 // Ray tracing extension
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,	 // Ray tracing extension
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME}; // Required by ray tracing pipeline;
    
    App* app;
    VkApp(App* _app);

        
    // @@ Here is a good place for variables managed by ImGui.  Grow as you wish.
    int frameCount;
    bool show_gui = true;
    bool useRaytracer = true;
    
    void drawFrame();
    bool acquireFrame();
    void renderGui();
    void drawGUI();
    void renderFrame();
    void submitFrame();

    void createAllVulkanResources();
    void destroyAllVulkanResources();
    void recreateSizedVulkanResources();

    // Some auxiliary functions
    VkCommandBuffer createTempCmdBuffer();
    void submitTempCmdBuffer(VkCommandBuffer cmdBuffer);
    VkShaderModule createShaderModuleFromFile(std::string filename);

    // Vulkan objects that will be created and the functions that will do the creation.
    VkInstance m_instance{};
    void createInstance();

    VkPhysicalDevice m_physicalDevice{};
    void createPhysicalDevice();

    uint32_t m_graphicsQueueIndex{VK_QUEUE_FAMILY_IGNORED};
    void chooseQueueIndex();

    VkDevice m_device{};
    void createDevice();

    VkQueue m_queue{};
    void getCommandQueue();
    
    void loadExtensions();
    
    VkSurfaceKHR m_surface{};
    void getSurface();
    
    VkCommandPool m_cmdPool{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkCommandBuffer m_commandBuffer; // A better name for this would be m_currentCommandBuffer
    void createCommandPool();

    VkFormat m_surfaceFormat       = VK_FORMAT_UNDEFINED;               // Temporary value.
    VkColorSpaceKHR m_surfaceColor = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // Temporary value
    VkSwapchainKHR m_swapchain{VK_NULL_HANDLE};
    uint32_t m_imageCount; // Set by createSwapchain, used often.
    int currentFrame = 0;
    std::vector<VkImage>     m_swapchainImages{};  // from vkGetSwapchainImagesKHR
    std::vector<VkImageView> m_imageViews{};
    std::vector<VkFence> m_inFlightFences{}; 
    std::vector<VkSemaphore> m_imageAvailableSemaphores{};
    std::vector<VkSemaphore> m_renderFinishedSemaphores{};

    void PrintBarrier(std::string id, VkImageMemoryBarrier& barrier);

    VkExtent2D m_windowSize{0, 0}; // Size of the window
    void createSwapchain();
    void destroySwapchain();

    ImageWrap m_depthImage;
    void createDepthResource();
    
    VkPipelineLayout m_postPipelineLayout{VK_NULL_HANDLE};
    
    void createPostFrameBuffers();

    VkPipeline m_postPipeline{VK_NULL_HANDLE};
    void createPostPipeline();
    
    #ifdef GUI
    ImGuiContext* m_ImGuiContext = nullptr;
    ImDrawData* draw_data;
    VkDescriptorPool m_imguiDescPool{VK_NULL_HANDLE};
    void initGUI();
    ImGui_ImplVulkanH_Window g_MainWindowData;
    #endif
    
    ImageWrap m_renderTarget{};
    void createRenderTarget();

    ImageWrap m_rtColCurrBuffer{}; 
    ImageWrap m_rtColPrevBuffer{};
    
    ImageWrap m_rtKdCurrBuffer{};
    ImageWrap m_rtKdPrevBuffer{};
    
    ImageWrap m_rtNdCurrBuffer{};
    ImageWrap m_rtNdPrevBuffer{};
    
    void createRtBuffers();
    void destroyRtBuffers();
    
    ImageWrap m_denoiseBuffer{};
    void createDenoiseBuffer();

    // Various model specific parameters
    vec3 scLightInt;
    vec3 scLightPos;
    vec3 scLightAmb;

    // Arrays of objects instances and textures in the scene
    std::vector<ObjData>  m_objData{};  // Obj data in Vulkan Buffers
    std::vector<ObjDesc>  m_objDesc{};  // Device-addresses of those buffers
    std::vector<ImageWrap>  m_objText{}; // All textures of the scene
    std::vector<ObjInst>  m_objInst{}; // Instances paring an object and a transform
    BufferWrap m_lightBuff{};          // Buffer of light list
    void createModel();                  // Load the scene.
    void destroyModel();
    bool readModel(const std::string& filename, glm::mat4 transform);

    BufferWrap m_objDescriptionBuff{};  // Device buffer of the OBJ descriptions
    void createObjDescriptionBuffer();

    DescriptorWrap m_scDesc{};
    void createScDescriptorSet();

    VkPipelineLayout            m_scPipelineLayout{};
    VkPipeline                  m_scPipeline{};
    void createScPipeline();

    BufferWrap m_matrixBuff{};  // Device-Host of the camera matrices
    void   createMatrixBuffer();
    
    float m_maxAnis = 0;
    PushConstantRay m_pcRay{};  // Push constant for ray tracer
    int m_num_atrous_iterations = 5;
    PushConstantDenoise m_pcDenoise{};
    uint32_t handleSize{};
    uint32_t handleAlignment{};
    uint32_t baseAlignment{};
    void initRayTracing();

    // Acceleration structure objects and functions
    BufferWrap m_scratch1;
    BufferWrap m_scratch2;
    RaytracingBuilderKHR m_rtBuilder{};
    BlasInput objectToVkGeometryKHR(const ObjData& model);
    void createBottomLevelAS();
    void createTopLevelAS();
    void createRtAccelerationStructure();

    // Raytrace descriptor set objects and functions
    DescriptorWrap m_rtDesc{};
    void createRtDescriptorSet();

    VkPipelineLayout m_rtPipelineLayout{};
    VkPipeline       m_rtPipeline{};
    void createRtPipeline();
    
    BufferWrap m_shaderBindingTableBuff;
    VkStridedDeviceAddressRegionKHR m_rgenRegion{};
    VkStridedDeviceAddressRegionKHR m_missRegion{};
    VkStridedDeviceAddressRegionKHR m_hitRegion{};
    VkStridedDeviceAddressRegionKHR m_callRegion{};
    void createRtShaderBindingTable();

    DescriptorWrap m_postDesc{};
    void createPostDescriptor();

    DescriptorWrap m_denoiseDesc{};
    void createDenoiseDescriptorSet();
    
    VkPipelineLayout m_denoiseCompPipelineLayout{};
    VkPipeline       m_denoisePipeline{};
    void createDenoiseCompPipeline();

    void CmdCopyImage(ImageWrap& src, ImageWrap& dst);

    void WrapPipelineBarrier(VkCommandBuffer cmd,
                             VkImage image,
                             VkImageAspectFlags aspectMask,
                             VkImageLayout oldLayout, VkImageLayout newLayout,
                             int srcAccessMask, int dstAccessMask,
                             int srcStage,  int dstStage);

    // Run loop 
    void ResetRtAccumulation();
    
    glm::mat4 m_priorViewProj{};
    void updateCameraBuffer();
    void rasterize();
    void raytrace();
    void denoise();
    
    uint32_t m_swapchainIndex{0};
    
    void postProcess();
    
     void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    
    // Various ways to create ImageWrap and BufferWrap structures. 
    void initImageWrap(ImageWrap& wrap, VkExtent2D& size,
                       VkFormat format,
                       VkImageUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkImageAspectFlagBits aspect,
                       VkImageLayout layout, 
                       uint32_t mipLevels=1);
    void initTextureSampler(ImageWrap& wrapper);

    ImageWrap readTextureFile(std::string fileName);
    void generateMipmap(VkImage image, VkFormat imageFormat,
                         int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    
    void initBufferWrap(BufferWrap& wrap, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties);

    
    void initBufferWrapFromData(BufferWrap& wrap,
                                const VkCommandBuffer& cmdBuf,
                                const VkDeviceSize&    size,
                                const void*            data,
                                VkBufferUsageFlags     usage);
    
    template <typename T>
    void initBufferWrapFromData(BufferWrap& wrap,
                              const VkCommandBuffer& cmdBuf,
                              const std::vector<T>&  data,
                              VkBufferUsageFlags     usage)
    {
        initBufferWrapFromData(wrap, cmdBuf, sizeof(T)*data.size(), data.data(), usage);
    }
    
};
