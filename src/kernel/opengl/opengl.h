#ifndef OPENGL_H
#define OPENGL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <QOpenGLFunctions>

// Предварительное объявление структур из kernel.h
struct Mesh;
struct Grid;
struct Camera;
struct Light;
struct WorldSettings;

class OpenGLRenderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    void init();
    void render(const Mesh& mesh, const Grid& grid, const Camera& camera,
                const std::vector<Light>& lights, const WorldSettings& settings, int displayMode);
    void updateProjection(float fov, float aspectRatio);
    void cleanup(Mesh& mesh, Grid& grid);
    Mesh loadModel(const std::string& path);
    Grid createGrid();

private:
    unsigned int shaderProgram;
    unsigned int gridShaderProgram;
};

namespace OpenGL {
    unsigned int compileShader(GLenum type, const char* source);
    unsigned int createShaderProgram(const char* vsSource, const char* fsSource);
    unsigned int loadTexture(const char* path);
    Grid createGrid();
    Mesh loadModel(const std::string& path);
    void updateProjection(unsigned int shaderProgram, unsigned int gridShaderProgram, float fov, float aspectRatio);
    void renderMesh(unsigned int shaderProgram, const Mesh& mesh, const glm::mat4& view, const glm::mat4& model,
                    const glm::vec3& cameraPos);
    void renderGrid(unsigned int gridShaderProgram, const Grid& grid, const glm::mat4& view);
    void init();
    void cleanup(Mesh& mesh, Grid& grid);
}

#endif // OPENGL_H