#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <QWindow>
#include <QVulkanInstance>

// Предварительное объявление структур из kernel.h
struct Mesh;
struct Grid;
struct Camera;
struct Light;
struct WorldSettings;

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    void init();
    void render(const Mesh& mesh, const Grid& grid, const Camera& camera,
                const std::vector<Light>& lights, const WorldSettings& settings, int displayMode);
    void cleanup(Mesh& mesh, Grid& grid);
    Mesh loadModel(const std::string& path);
    Grid createGrid();
    void updateProjection(float fov, float aspectRatio);

    // Методы для интеграции с QVulkanWindow
    void setVulkanInstance(QVulkanInstance* instance);
    void setSurface(VkSurfaceKHR surface);
    void setPhysicalDevice(VkPhysicalDevice physicalDevice);
    void setDevice(VkDevice device);

private:
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSwapchain();
    void cleanupSwapchain();
    void cleanupVulkan();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readFile(const std::string& filename);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    QVulkanInstance* instance; // Qt Vulkan instance
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> framebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrameIndex;
    VkBuffer meshVertexBuffer;
    VkDeviceMemory meshVertexBufferMemory;
    VkBuffer meshIndexBuffer;
    VkDeviceMemory meshIndexBufferMemory;
    VkBuffer gridVertexBuffer;
    VkDeviceMemory gridVertexBufferMemory;
    VkBuffer gridIndexBuffer;
    VkDeviceMemory gridIndexBufferMemory;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    VkSurfaceKHR surface;
    uint32_t graphicsFamily;
    uint32_t presentFamily;
};

#endif // VULKAN_RENDERER_H