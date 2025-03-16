#ifndef DIRECT3D_RENDERER_H
#define DIRECT3D_RENDERER_H

#ifdef _WIN32 // Условная компиляция для Windows
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <glm/glm.hpp>
#include <vector>

// Предварительное объявление структур из kernel.h
struct Mesh;
struct Grid;
struct Camera;
struct Light;
struct WorldSettings;

class Direct3DRenderer {
public:
    Direct3DRenderer();
    ~Direct3DRenderer();

    void init();
    void render(const Mesh& mesh, const Grid& grid, const Camera& camera,
                const std::vector<Light>& lights, const WorldSettings& settings, int displayMode);
    void cleanup(Mesh& mesh, Grid& grid);
    Mesh loadModel(const std::string& path);
    Grid createGrid();
    void updateProjection(float fov, float aspectRatio);
    HWND getWindow() const { return hWnd; } // Direct3D использует HWND

private:
    void createDeviceAndSwapChain();
    void createRenderTargetView();
    void createDepthStencilView();
    void createShaders();
    void setupConstantBuffers();
    void setupMeshBuffers(const Mesh& mesh);
    void setupGridBuffers(const Grid& grid);

    HWND hWnd;                           // Дескриптор окна Windows
    ID3D11Device* device;                // Устройство Direct3D
    ID3D11DeviceContext* context;        // Контекст устройства
    IDXGISwapChain* swapChain;           // Цепочка обмена
    ID3D11RenderTargetView* renderTargetView; // Вид цели рендеринга
    ID3D11DepthStencilView* depthStencilView; // Вид глубины/трафарета
    ID3D11VertexShader* vertexShader;    // Вершинный шейдер
    ID3D11PixelShader* pixelShader;      // Пиксельный шейдер
    ID3D11InputLayout* inputLayout;      // Макет ввода
    ID3D11Buffer* meshVertexBuffer;      // Буфер вершин для меша
    ID3D11Buffer* meshIndexBuffer;       // Буфер индексов для меша
    ID3D11Buffer* gridVertexBuffer;      // Буфер вершин для сетки
    ID3D11Buffer* gridIndexBuffer;       // Буфер индексов для сетки
    ID3D11Buffer* constantBuffer;        // Константы для шейдеров (model, view, proj, etc.)
    ID3D11Buffer* materialBuffer;        // Буфер материала
    ID3D11Buffer* lightBuffer;           // Буфер света
    float currentAspectRatio;            // Текущий аспект для проекции
};
#else
// Заглушка для не-Windows платформ
class Direct3DRenderer {
public:
    Direct3DRenderer() { throw std::runtime_error("Direct3D is not supported on this platform"); }
    void init() {}
    void render(const Mesh&, const Grid&, const Camera&, const std::vector<Light>&, const WorldSettings&, int) {}
    void cleanup(Mesh&, Grid&) {}
    Mesh loadModel(const std::string&) { return Mesh(); }
    Grid createGrid() { return Grid(); }
    void updateProjection(float, float) {}
    void* getWindow() const { return nullptr; }
};
#endif

#endif // DIRECT3D_RENDERER_H