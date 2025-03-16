#include "kernel.h"
#include "opengl/opengl.h"
#include "vulkan/vulkan.h"
#ifdef _WIN32
#include "directx/directx.h"
#endif

Kernel::Kernel() : openGLRenderer(nullptr), vulkanRenderer(nullptr), direct3DRenderer(nullptr),
                   currentFov(45.0f), currentAspectRatio(1.0f), renderAPI(RenderAPI::OpenGL) {
    camera.Position = glm::vec3(0.0f, 0.0f, 5.0f);
    camera.Front = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.Fov = 45.0f;

    settings.ambientLight = glm::vec3(0.2f);

    Light light;
    light.type = 1; // Sun light
    light.position = glm::vec3(0.0f);
    light.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    light.color = glm::vec3(1.0f);
    light.intensity = 1.0f;
    lights.push_back(light);
}

Kernel::~Kernel() {
    if (vulkanRenderer) {
        vulkanRenderer->cleanup(mesh, grid);
        delete vulkanRenderer;
    }
    if (openGLRenderer) {
        openGLRenderer->cleanup(mesh, grid);
        delete openGLRenderer;
    }
    if (direct3DRenderer) {
        direct3DRenderer->cleanup(mesh, grid);
        delete direct3DRenderer;
    }
}

void Kernel::init() {
    if (renderAPI == RenderAPI::Vulkan && vulkanRenderer) {
        vulkanRenderer->init();
    } else if (renderAPI == RenderAPI::Direct3D && direct3DRenderer) {
        direct3DRenderer->init();
    } else if (openGLRenderer) {
        openGLRenderer->init();
    }
}

void Kernel::render(int displayMode) {
    if (renderAPI == RenderAPI::Vulkan && vulkanRenderer) {
        vulkanRenderer->render(mesh, grid, camera, lights, settings, displayMode);
    } else if (renderAPI == RenderAPI::Direct3D && direct3DRenderer) {
        direct3DRenderer->render(mesh, grid, camera, lights, settings, displayMode);
    } else if (openGLRenderer) {
        openGLRenderer->render(mesh, grid, camera, lights, settings, displayMode);
    }
}

void Kernel::updateProjection(float fov, float aspectRatio) {
    currentFov = fov;
    currentAspectRatio = aspectRatio;
    if (renderAPI == RenderAPI::Vulkan && vulkanRenderer) {
        vulkanRenderer->updateProjection(fov, aspectRatio);
    } else if (renderAPI == RenderAPI::Direct3D && direct3DRenderer) {
        direct3DRenderer->updateProjection(fov, aspectRatio);
    } else if (openGLRenderer) {
        openGLRenderer->updateProjection(fov, aspectRatio);
    }
}

void Kernel::loadModel(const std::string& path) {
    if (renderAPI == RenderAPI::Vulkan && vulkanRenderer) {
        mesh = vulkanRenderer->loadModel(path);
    } else if (renderAPI == RenderAPI::Direct3D && direct3DRenderer) {
        mesh = direct3DRenderer->loadModel(path);
    } else if (openGLRenderer) {
        mesh = openGLRenderer->loadModel(path);
    }
}

void Kernel::createGrid() {
    if (renderAPI == RenderAPI::Vulkan && vulkanRenderer) {
        grid = vulkanRenderer->createGrid();
    } else if (renderAPI == RenderAPI::Direct3D && direct3DRenderer) {
        grid = direct3DRenderer->createGrid();
    } else if (openGLRenderer) {
        grid = openGLRenderer->createGrid();
    }
}

void Kernel::setOpenGLRenderer(OpenGLRenderer* renderer) {
    openGLRenderer = renderer;
    renderAPI = RenderAPI::OpenGL;
}

void Kernel::setVulkanRenderer(VulkanRenderer* renderer) {
    vulkanRenderer = renderer;
    renderAPI = RenderAPI::Vulkan;
}

void Kernel::setDirect3DRenderer(Direct3DRenderer* renderer) {
    direct3DRenderer = renderer;
    renderAPI = RenderAPI::Direct3D;
}

void Kernel::setVulkanInstance(QVulkanInstance* instance) {
    if (vulkanRenderer) {
        vulkanRenderer->setVulkanInstance(instance);
    }
}

void Kernel::setVulkanSurface(VkSurfaceKHR surface) {
    if (vulkanRenderer) {
        vulkanRenderer->setSurface(surface);
    }
}

void Kernel::setVulkanPhysicalDevice(VkPhysicalDevice physicalDevice) {
    if (vulkanRenderer) {
        vulkanRenderer->setPhysicalDevice(physicalDevice);
    }
}

void Kernel::setVulkanDevice(VkDevice device) {
    if (vulkanRenderer) {
        vulkanRenderer->setDevice(device);
    }
}