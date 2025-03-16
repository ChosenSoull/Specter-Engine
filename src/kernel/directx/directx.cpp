#ifdef _WIN32 // Реализация только для Windows
#include "directx.h"
#include <stdexcept>
#include <d3dcompiler.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../kernel.h" // Для доступа к Mesh, Grid, Camera, Light, WorldSettings
#include "../opengl/opengl.h" // Для повторного использования загрузки модели

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// HLSL шейдеры (адаптация GLSL)
const char* vertexShaderSource = R"hlsl(
    cbuffer ConstantBuffer : register(b0) {
        float4x4 model;
        float4x4 view;
        float4x4 projection;
    };

    struct VS_INPUT {
        float3 aPos : POSITION;
        float3 aNormal : NORMAL;
    };

    struct VS_OUTPUT {
        float4 Pos : SV_POSITION;
        float3 FragPos : POSITION;
        float3 Normal : NORMAL;
    };

    VS_OUTPUT main(VS_INPUT input) {
        VS_OUTPUT output;
        output.FragPos = mul(float4(input.aPos, 1.0), model).xyz;
        output.Normal = mul(float3x3(transpose(inverse(model))), input.aNormal);
        output.Pos = mul(mul(mul(float4(input.aPos, 1.0), model), view), projection);
        return output;
    }
)hlsl";

const char* pixelShaderSource = R"hlsl(
    cbuffer ConstantBuffer : register(b0) {
        float4x4 model;
        float4x4 view;
        float4x4 projection;
        float3 viewPos;
        int displayMode;
        float3 worldAmbient;
        int numLights;
    };

    cbuffer MaterialBuffer : register(b1) {
        float3 ambient;
        float3 diffuse;
        float3 specular;
        float shininess;
    };

    cbuffer LightBuffer : register(b2) {
        struct Light {
            int type;
            float3 position;
            float3 direction;
            float3 color;
            float intensity;
            float spotAngle;
            float2 size;
            float2 padding;
        } lights[4];
    };

    struct PS_INPUT {
        float4 Pos : SV_POSITION;
        float3 FragPos : POSITION;
        float3 Normal : NORMAL;
    };

    float3 calcPointLight(Light light, float3 normal, float3 fragPos, float3 viewDir) {
        float3 lightDir = normalize(light.position - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        float3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (distance * distance);
        return light.color * (ambient + diff * diffuse + spec * specular) * light.intensity * attenuation;
    }

    float3 calcSunLight(Light light, float3 normal, float3 viewDir) {
        float3 lightDir = normalize(-light.direction);
        float diff = max(dot(normal, lightDir), 0.0);
        float3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        return light.color * (ambient * 0.1 + diff * diffuse + spec * specular) * light.intensity;
    }

    float3 calcSpotLight(Light light, float3 normal, float3 fragPos, float3 viewDir) {
        float3 lightDir = normalize(light.position - fragPos);
        float theta = dot(lightDir, normalize(-light.direction));
        float cutoff = cos(radians(light.spotAngle));
        if (theta > cutoff) {
            float diff = max(dot(normal, lightDir), 0.0);
            float3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            float distance = length(light.position - fragPos);
            float attenuation = 1.0 / (distance * distance);
            return light.color * (ambient + diff * diffuse + spec * specular) * light.intensity * attenuation;
        }
        return float3(0.0, 0.0, 0.0);
    }

    float3 calcAreaLight(Light light, float3 normal, float3 fragPos, float3 viewDir) {
        float3 lightDir = normalize(light.position - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        float3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (distance * distance) * light.size.x * light.size.y;
        return light.color * (ambient + diff * diffuse + spec * specular) * light.intensity * attenuation;
    }

    float4 main(PS_INPUT input) : SV_TARGET {
        float3 norm = normalize(input.Normal);
        float3 viewDir = normalize(viewPos - input.FragPos);
        float3 result = float3(0.0, 0.0, 0.0);

        if (displayMode == 0) {
            result = diffuse;
        } else {
            result += worldAmbient * ambient;
            for (int i = 0; i < numLights && i < 4; i++) {
                if (lights[i].type == 0) result += calcPointLight(lights[i], norm, input.FragPos, viewDir);
                else if (lights[i].type == 1) result += calcSunLight(lights[i], norm, viewDir);
                else if (lights[i].type == 2) result += calcSpotLight(lights[i], norm, input.FragPos, viewDir);
                else if (lights[i].type == 3) result += calcAreaLight(lights[i], norm, input.FragPos, viewDir);
            }
        }
        return float4(result, 1.0);
    }
)hlsl";

Direct3DRenderer::Direct3DRenderer() : 
    hWnd(nullptr), device(nullptr), context(nullptr), swapChain(nullptr), 
    renderTargetView(nullptr), depthStencilView(nullptr), vertexShader(nullptr), 
    pixelShader(nullptr), inputLayout(nullptr), meshVertexBuffer(nullptr), 
    meshIndexBuffer(nullptr), gridVertexBuffer(nullptr), gridIndexBuffer(nullptr), 
    constantBuffer(nullptr), materialBuffer(nullptr), lightBuffer(nullptr),
    currentAspectRatio(1280.0f / 720.0f) {}

Direct3DRenderer::~Direct3DRenderer() {
    cleanup(Mesh(), Grid());
}

void Direct3DRenderer::init() {
    // Создание окна через WinAPI
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"Direct3DWindow";
    RegisterClass(&wc);

    hWnd = CreateWindow(L"Direct3DWindow", L"Blender-like Viewer (Direct3D)", 
                        WS_OVERLAPPEDWINDOW, 0, 0, 1280, 720, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hWnd) {
        throw std::runtime_error("Failed to create window");
    }
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Создание устройства и цепочки обмена
    createDeviceAndSwapChain();
    createRenderTargetView();
    createDepthStencilView();
    createShaders();
    setupConstantBuffers();

    // Включение теста глубины
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    ID3D11DepthStencilState* depthStencilState;
    device->CreateDepthStencilState(&dsDesc, &depthStencilState);
    context->OMSetDepthStencilState(depthStencilState, 1);
    depthStencilState->Release();
}

void Direct3DRenderer::createDeviceAndSwapChain() {
    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferCount = 1;
    scDesc.BufferDesc.Width = 1280;
    scDesc.BufferDesc.Height = 720;
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.OutputWindow = hWnd;
    scDesc.SampleDesc.Count = 1;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, 
                                               nullptr, 0, D3D11_SDK_VERSION, &scDesc, 
                                               &swapChain, &device, nullptr, &context);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create Direct3D device and swap chain");
    }
}

void Direct3DRenderer::createRenderTargetView() {
    ID3D11Texture2D* backBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    device->CreateRenderTargetView(backBuffer, nullptr, &renderTargetView);
    backBuffer->Release();
}

void Direct3DRenderer::createDepthStencilView() {
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = 1280;
    depthDesc.Height = 720;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthStencil;
    device->CreateTexture2D(&depthDesc, nullptr, &depthStencil);
    device->CreateDepthStencilView(depthStencil, nullptr, &depthStencilView);
    depthStencil->Release();

    context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
}

void Direct3DRenderer::createShaders() {
    ID3DBlob* vsBlob, *psBlob;
    HRESULT hr;

    hr = D3DCompile(vertexShaderSource, strlen(vertexShaderSource), nullptr, nullptr, nullptr, 
                    "main", "vs_4_0", 0, 0, &vsBlob, nullptr);
    if (FAILED(hr)) throw std::runtime_error("Failed to compile vertex shader");

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) throw std::runtime_error("Failed to create vertex shader");

    hr = D3DCompile(pixelShaderSource, strlen(pixelShaderSource), nullptr, nullptr, nullptr, 
                    "main", "ps_4_0", 0, 0, &psBlob, nullptr);
    if (FAILED(hr)) throw std::runtime_error("Failed to compile pixel shader");

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) throw std::runtime_error("Failed to create pixel shader");

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);

    vsBlob->Release();
    psBlob->Release();
}

void Direct3DRenderer::setupConstantBuffers() {
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(glm::mat4) * 3 + sizeof(glm::vec3) + sizeof(int) * 2 + sizeof(glm::vec3); // model, view, proj, viewPos, displayMode, numLights, worldAmbient
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);

    cbDesc.ByteWidth = sizeof(glm::vec3) * 3 + sizeof(float); // ambient, diffuse, specular, shininess
    device->CreateBuffer(&cbDesc, nullptr, &materialBuffer);

    cbDesc.ByteWidth = sizeof(int) + sizeof(glm::vec3) * 3 + sizeof(float) * 4; // type, position, direction, color, intensity, spotAngle, size, padding
    cbDesc.ByteWidth *= 4; // Для 4 источников света
    device->CreateBuffer(&cbDesc, nullptr, &lightBuffer);
}

void Direct3DRenderer::setupMeshBuffers(const Mesh& mesh) {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
    };
    std::vector<Vertex> vertices;
    for (size_t i = 0; i < mesh.positions.size(); i++) {
        Vertex vertex;
        vertex.position = mesh.positions[i];
        vertex.normal = mesh.normals[i];
        vertices.push_back(vertex);
    }

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices.data();
    device->CreateBuffer(&vbDesc, &vbData, &meshVertexBuffer);

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = static_cast<UINT>(mesh.indices.size() * sizeof(unsigned int));
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = mesh.indices.data();
    device->CreateBuffer(&ibDesc, &ibData, &meshIndexBuffer);
}

void Direct3DRenderer::setupGridBuffers(const Grid& grid) {
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(grid.vertices.size() * sizeof(glm::vec3));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = grid.vertices.data();
    device->CreateBuffer(&vbDesc, &vbData, &gridVertexBuffer);

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = static_cast<UINT>(grid.indices.size() * sizeof(unsigned int));
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = grid.indices.data();
    device->CreateBuffer(&ibDesc, &ibData, &gridIndexBuffer);
}

void Direct3DRenderer::render(const Mesh& mesh, const Grid& grid, const Camera& camera, 
                              const std::vector<Light>& lights, const WorldSettings& settings, int displayMode) {
    float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    context->ClearRenderTargetView(renderTargetView, clearColor);
    context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    context->IASetInputLayout(inputLayout);
    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(pixelShader, nullptr, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Обновление константного буфера
    D3D11_MAPPED_SUBRESOURCE mapped;
    context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    struct ConstantBuffer {
        glm::mat4 model, view, projection;
        glm::vec3 viewPos;
        int displayMode, numLights;
        glm::vec3 worldAmbient;
    } cbData;
    cbData.model = glm::mat4(1.0f);
    cbData.view = glm::lookAt(camera.Position, camera.Position + camera.Front, camera.Up);
    cbData.projection = glm::perspective(glm::radians(camera.Fov), currentAspectRatio, 0.1f, 100.0f);
    cbData.viewPos = camera.Position;
    cbData.displayMode = displayMode;
    cbData.numLights = static_cast<int>(lights.size());
    cbData.worldAmbient = settings.ambientLight;
    memcpy(mapped.pData, &cbData, sizeof(cbData));
    context->Unmap(constantBuffer, 0);

    // Обновление буфера материала
    context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    struct MaterialBuffer {
        glm::vec3 ambient, diffuse, specular;
        float shininess;
    } matData;
    matData.ambient = mesh.material.ambient;
    matData.diffuse = mesh.material.diffuse;
    matData.specular = mesh.material.specular;
    matData.shininess = mesh.material.shininess;
    memcpy(mapped.pData, &matData, sizeof(matData));
    context->Unmap(materialBuffer, 0);

    // Обновление буфера света
    context->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    struct LightData {
        int type;
        glm::vec3 position;
        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        float spotAngle;
        glm::vec2 size;
        glm::vec2 padding;
    } lightData[4];
    for (size_t i = 0; i < lights.size() && i < 4; i++) {
        lightData[i].type = lights[i].type;
        lightData[i].position = lights[i].position;
        lightData[i].direction = lights[i].direction;
        lightData[i].color = lights[i].color;
        lightData[i].intensity = lights[i].intensity;
        lightData[i].spotAngle = lights[i].spotAngle;
        lightData[i].size = lights[i].size;
        lightData[i].padding = glm::vec2(0.0f);
    }
    memcpy(mapped.pData, lightData, sizeof(lightData));
    context->Unmap(lightBuffer, 0);

    ID3D11Buffer* constantBuffers[] = { constantBuffer, materialBuffer, lightBuffer };
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetConstantBuffers(0, 3, constantBuffers);

    // Рендеринг меша
    if (meshVertexBuffer && meshIndexBuffer) {
        UINT stride = sizeof(glm::vec3) * 2; // Position + Normal
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &meshVertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(meshIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
    }

    // Рендеринг сетки (временная заглушка, нужно отдельный шейдер для линий)
    if (gridVertexBuffer && gridIndexBuffer) {
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        UINT stride = sizeof(glm::vec3);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &gridVertexBuffer, &stride, &offset);
        context->IASetIndexBuffer(gridIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
        context->DrawIndexed(static_cast<UINT>(grid.indices.size()), 0, 0);
    }

    swapChain->Present(0, 0);
}

void Direct3DRenderer::cleanup(Mesh& mesh, Grid& grid) {
    if (lightBuffer) lightBuffer->Release();
    if (materialBuffer) materialBuffer->Release();
    if (constantBuffer) constantBuffer->Release();
    if (meshVertexBuffer) meshVertexBuffer->Release();
    if (meshIndexBuffer) meshIndexBuffer->Release();
    if (gridVertexBuffer) gridVertexBuffer->Release();
    if (gridIndexBuffer) gridIndexBuffer->Release();
    if (inputLayout) inputLayout->Release();
    if (pixelShader) pixelShader->Release();
    if (vertexShader) vertexShader->Release();
    if (depthStencilView) depthStencilView->Release();
    if (renderTargetView) renderTargetView->Release();
    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    if (hWnd) DestroyWindow(hWnd);
}

Mesh Direct3DRenderer::loadModel(const std::string& path) {
    Mesh mesh = OpenGL::loadModel(path); // Повторное использование загрузки из OpenGL
    if (meshVertexBuffer) meshVertexBuffer->Release();
    if (meshIndexBuffer) meshIndexBuffer->Release();
    setupMeshBuffers(mesh);
    return mesh;
}

Grid Direct3DRenderer::createGrid() {
    Grid grid = OpenGL::createGrid(); // Повторное использование создания сетки из OpenGL
    if (gridVertexBuffer) gridVertexBuffer->Release();
    if (gridIndexBuffer) gridIndexBuffer->Release();
    setupGridBuffers(grid);
    return grid;
}

void Direct3DRenderer::updateProjection(float fov, float aspectRatio) {
    currentAspectRatio = aspectRatio;
}
#endif // Конец условной компиляции для Windows