#include "opengl.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "../../../filesbuild/stb_image.h"

#define CHECK_GL_ERROR() {\
    GLenum err = glGetError();\
    if(err != GL_NO_ERROR) {\
        std::cerr << "OpenGL error: " << err << " at line " << __LINE__ << std::endl;\
    }\
}

const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    out vec3 FragPos;
    out vec3 Normal;
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec3 FragPos;
    in vec3 Normal;
    uniform vec3 viewPos;
    uniform int displayMode;
    uniform struct Material {
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
        float shininess;
    } material;
    uniform vec3 worldAmbient;
    #define MAX_LIGHTS 4
    uniform struct Light {
        int type;
        vec3 position;
        vec3 direction;
        vec3 color;
        float intensity;
        float spotAngle;
        vec2 size;
    } lights[MAX_LIGHTS];
    uniform unsigned int numLights;

    vec3 calcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
        vec3 lightDir = normalize(light.position - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 ambient = light.color * material.ambient * light.intensity * attenuation;
        vec3 diffuse = light.color * (diff * material.diffuse) * light.intensity * attenuation;
        vec3 specular = light.color * (spec * material.specular) * light.intensity * attenuation;
        return ambient + diffuse + specular;
    }

    vec3 calcSunLight(Light light, vec3 normal, vec3 viewDir) {
        vec3 lightDir = normalize(-light.direction);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 ambient = light.color * material.ambient * light.intensity * 0.1;
        vec3 diffuse = light.color * (diff * material.diffuse) * light.intensity;
        vec3 specular = light.color * (spec * material.specular) * light.intensity;
        return ambient + diffuse + specular;
    }

    vec3 calcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
        vec3 lightDir = normalize(light.position - fragPos);
        float theta = dot(lightDir, normalize(-light.direction));
        float cutoff = cos(radians(light.spotAngle));
        if (theta > cutoff) {
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
            float distance = length(light.position - fragPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 ambient = light.color * material.ambient * light.intensity * attenuation;
            vec3 diffuse = light.color * (diff * material.diffuse) * light.intensity * attenuation;
            vec3 specular = light.color * (spec * material.specular) * light.intensity * attenuation;
            return ambient + diffuse + specular;
        }
        return vec3(0.0);
    }

    vec3 calcAreaLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
        vec3 lightDir = normalize(light.position - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (distance * distance) * light.size.x * light.size.y;
        vec3 ambient = light.color * material.ambient * light.intensity * attenuation;
        vec3 diffuse = light.color * (diff * material.diffuse) * light.intensity * attenuation;
        vec3 specular = light.color * (spec * material.specular) * light.intensity * attenuation;
        return ambient + diffuse + specular;
    }

    void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 result = vec3(0.0);

        if (displayMode == 0) {
            result = material.diffuse;
        } else {
            result += worldAmbient * material.ambient;
            for (unsigned int i = 0; i < numLights && i < MAX_LIGHTS; i++) {
                if (lights[i].type == 0) result += calcPointLight(lights[i], norm, FragPos, viewDir);
                else if (lights[i].type == 1) result += calcSunLight(lights[i], norm, viewDir);
                else if (lights[i].type == 2) result += calcSpotLight(lights[i], norm, FragPos, viewDir);
                else if (lights[i].type == 3) result += calcAreaLight(lights[i], norm, FragPos, viewDir);
            }
        }
        FragColor = vec4(result, 1.0);
    }
)glsl";

const char* gridVertexShader = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 view;
    uniform mat4 projection;
    void main() { gl_Position = projection * view * vec4(aPos, 1.0); }
)glsl";

const char* gridFragmentShader = R"glsl(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() { FragColor = vec4(color, 1.0); }
)glsl";

OpenGLRenderer::OpenGLRenderer() : shaderProgram(0), gridShaderProgram(0) {}

OpenGLRenderer::~OpenGLRenderer() {}

void OpenGLRenderer::init() {
    initializeOpenGLFunctions();

    OpenGL::init();

    shaderProgram = OpenGL::createShaderProgram(vertexShaderSource, fragmentShaderSource);
    gridShaderProgram = OpenGL::createShaderProgram(gridVertexShader, gridFragmentShader);
}

void OpenGLRenderer::render(const Mesh& mesh, const Grid& grid, const Camera& camera,
                            const std::vector<Light>& lights, const WorldSettings& settings, int displayMode) {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = glm::lookAt(camera.Position, camera.Position + camera.Front, camera.Up);
    glm::mat4 model = glm::mat4(1.0f);

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &camera.Position[0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "displayMode"), displayMode);
    glUniform3fv(glGetUniformLocation(shaderProgram, "worldAmbient"), 1, &settings.ambientLight[0]);
    glUniform1ui(glGetUniformLocation(shaderProgram, "numLights"), static_cast<unsigned int>(lights.size()));
    for (size_t i = 0; i < lights.size() && i < 4; ++i) {
        std::string base = "lights[" + std::to_string(i) + "].";
        glUniform1i(glGetUniformLocation(shaderProgram, (base + "type").c_str()), lights[i].type);
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + "position").c_str()), 1, &lights[i].position[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + "direction").c_str()), 1, &lights[i].direction[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, (base + "color").c_str()), 1, &lights[i].color[0]);
        glUniform1f(glGetUniformLocation(shaderProgram, (base + "intensity").c_str()), lights[i].intensity);
        glUniform1f(glGetUniformLocation(shaderProgram, (base + "spotAngle").c_str()), lights[i].spotAngle);
        glUniform2fv(glGetUniformLocation(shaderProgram, (base + "size").c_str()), 1, &lights[i].size[0]);
    }
    glUniform3fv(glGetUniformLocation(shaderProgram, "material.ambient"), 1, &mesh.material.ambient[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "material.diffuse"), 1, &mesh.material.diffuse[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "material.specular"), 1, &mesh.material.specular[0]);
    glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), mesh.material.shininess);
    OpenGL::renderMesh(shaderProgram, mesh, view, model, camera.Position);

    OpenGL::renderGrid(gridShaderProgram, grid, view);
}

void OpenGLRenderer::updateProjection(float fov, float aspectRatio) {
    OpenGL::updateProjection(shaderProgram, gridShaderProgram, fov, aspectRatio);
}

void OpenGLRenderer::cleanup(Mesh& mesh, Grid& grid) {
    OpenGL::cleanup(mesh, grid);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(gridShaderProgram);
}

Mesh OpenGLRenderer::loadModel(const std::string& path) {
    return OpenGL::loadModel(path);
}

Grid OpenGLRenderer::createGrid() {
    return OpenGL::createGrid();
}

namespace OpenGL {
    unsigned int compileShader(GLenum type, const char* source) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation error: " << infoLog << std::endl;
        }
        return shader;
    }

    unsigned int createShaderProgram(const char* vsSource, const char* fsSource) {
        unsigned int program = glCreateProgram();
        unsigned int vs = compileShader(GL_VERTEX_SHADER, vsSource);
        unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fsSource);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Program linking error: " << infoLog << std::endl;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }

    unsigned int loadTexture(const char* path) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrComponents == 1) format = GL_RED;
            else if (nrComponents == 4) format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        } else {
            std::cerr << "Failed to load texture: " << path << std::endl;
        }
        return textureID;
    }

    Grid createGrid() {
        Grid grid;
        const int gridSize = 20;
        const float step = 1.0f;

        for (int i = -gridSize; i <= gridSize; ++i) {
            grid.vertices.emplace_back(i * step, 0.0f, -gridSize * step);
            grid.vertices.emplace_back(i * step, 0.0f, gridSize * step);
            grid.vertices.emplace_back(-gridSize * step, 0.0f, i * step);
            grid.vertices.emplace_back(gridSize * step, 0.0f, i * step);
        }

        for (unsigned int i = 0; i < grid.vertices.size(); i++)
            grid.indices.push_back(i);

        glGenVertexArrays(1, &grid.VAO);
        glGenBuffers(1, &grid.VBO);
        glGenBuffers(1, &grid.EBO);

        glBindVertexArray(grid.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, grid.VBO);
        glBufferData(GL_ARRAY_BUFFER, grid.vertices.size() * sizeof(glm::vec3), grid.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grid.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, grid.indices.size() * sizeof(unsigned int), grid.indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        return grid;
    }

    Mesh loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
            return Mesh(); // Возвращаем пустой меш вместо выхода
        }

        aiMesh* aiMesh = scene->mMeshes[0];
        Mesh mesh;
        mesh.bboxMin = glm::vec3(FLT_MAX);
        mesh.bboxMax = glm::vec3(-FLT_MAX);

        for (unsigned int i = 0; i < aiMesh->mNumVertices; i++) {
            glm::vec3 position(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);
            mesh.positions.push_back(position);
            mesh.bboxMin = glm::min(mesh.bboxMin, position);
            mesh.bboxMax = glm::max(mesh.bboxMax, position);

            if (aiMesh->HasNormals()) {
                mesh.normals.emplace_back(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
            }
        }

        for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
            aiFace face = aiMesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                mesh.indices.push_back(face.mIndices[j]);
        }

        if (aiMesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
            aiColor3D color(0.0f, 0.0f, 0.0f);
            material->Get(AI_MATKEY_COLOR_AMBIENT, color);
            mesh.material.ambient = glm::vec3(color.r, color.g, color.b);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            mesh.material.diffuse = glm::vec3(color.r, color.g, color.b);
            material->Get(AI_MATKEY_COLOR_SPECULAR, color);
            mesh.material.specular = glm::vec3(color.r, color.g, color.b);
            material->Get(AI_MATKEY_SHININESS, mesh.material.shininess);
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString path;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
                mesh.material.diffuseTexture = loadTexture(path.C_Str());
            }
        }

        glGenVertexArrays(1, &mesh.VAO);
        glGenBuffers(2, mesh.VBO);
        glGenBuffers(1, &mesh.EBO);

        glBindVertexArray(mesh.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, mesh.positions.size() * sizeof(glm::vec3), mesh.positions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, mesh.normals.size() * sizeof(glm::vec3), mesh.normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

        return mesh;
    }

    void updateProjection(unsigned int shaderProgram, unsigned int gridShaderProgram, float fov, float aspectRatio) {
        glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glUseProgram(gridShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    }

    void renderMesh(unsigned int shaderProgram, const Mesh& mesh, const glm::mat4& view, const glm::mat4& model,
                    const glm::vec3& cameraPos) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, &cameraPos[0]);

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
    }

    void renderGrid(unsigned int gridShaderProgram, const Grid& grid, const glm::mat4& view) {
        glUseProgram(gridShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniform3f(glGetUniformLocation(gridShaderProgram, "color"), 0.3f, 0.3f, 0.3f);
        glBindVertexArray(grid.VAO);
        glDrawElements(GL_LINES, grid.indices.size(), GL_UNSIGNED_INT, 0);
    }

    void init() {
        glEnable(GL_DEPTH_TEST);
    }

    void cleanup(Mesh& mesh, Grid& grid) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(2, mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
        glDeleteVertexArrays(1, &grid.VAO);
        glDeleteBuffers(1, &grid.VBO);
        glDeleteBuffers(1, &grid.EBO);
    }
}