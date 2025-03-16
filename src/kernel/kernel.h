#ifndef KERNEL_H
#define KERNEL_H

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <QObject>

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    unsigned int diffuseTexture;
};

struct Mesh {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;
    Material material;
    glm::vec3 bboxMin;
    glm::vec3 bboxMax;
    unsigned int VAO;
    unsigned int VBO[2];
    unsigned int EBO;
};

struct Grid {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};

struct Camera {
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    float Fov;
};

struct Light {
    int type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float spotAngle;
    glm::vec2 size;
};

struct WorldSettings {
    glm::vec3 ambientLight;
};

class OpenGLRenderer;
class VulkanRenderer;
class Direct3DRenderer;

class Kernel : public QObject {
    Q_OBJECT
public:
    Kernel();
    ~Kernel();

    void init();
    void render(int displayMode);
    void updateProjection(float fov, float aspectRatio);
    void loadModel(const std::string& path);
    void createGrid();

    // Методы для установки рендерера
    void setOpenGLRenderer(OpenGLRenderer* renderer);
    void setVulkanRenderer(VulkanRenderer* renderer);
    void setDirect3DRenderer(Direct3DRenderer* renderer);
    void setVulkanInstance(QVulkanInstance* instance);
    void setVulkanSurface(VkSurfaceKHR surface);
    void setVulkanPhysicalDevice(VkPhysicalDevice physicalDevice);
    void setVulkanDevice(VkDevice device);

private:
    Mesh mesh;
    Grid grid;
    Camera camera;
    std::vector<Light> lights;
    WorldSettings settings;
    OpenGLRenderer* openGLRenderer;
    VulkanRenderer* vulkanRenderer;
    Direct3DRenderer* direct3DRenderer;
    float currentFov;
    float currentAspectRatio;
    enum class RenderAPI { OpenGL, Vulkan, Direct3D } renderAPI;
};

#endif // KERNEL_H