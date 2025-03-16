cmake_minimum_required(VERSION 3.10)
project(SpecterEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)  # Для ресурсов
set(CMAKE_AUTOUIC ON)

# Поиск Qt с необходимыми компонентами
find_package(Qt5 COMPONENTS Widgets Network OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)

# Проверка наличия Vulkan
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found! Please install Vulkan SDK.")
endif()

# Проверка наличия Qt5
if (NOT Qt5_FOUND)
    message(FATAL_ERROR "Qt5 not found! Please install Qt5 with Widgets, Network, and OpenGL components.")
endif()

# Включение директорий
include_directories(
    ${GLFW_INCLUDE_DIRS}
    ${ASSIMP_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Ресурсы
qt5_add_resources(RESOURCES resources.qrc)

# Библиотека glad
add_library(glad STATIC filesbuild/src/glad.c)
target_include_directories(glad PUBLIC include)

# Библиотека stb_image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# UI библиотека
file(GLOB UI_SRC
    "src/UI/*.cpp"
    "src/UI/*.h"  # Добавляем заголовочные файлы для AUTOMOC
)
add_library(ui STATIC ${UI_SRC} ${RESOURCES})
target_link_libraries(ui
    Qt5::Widgets
    Qt5::Network
    Qt5::OpenGL  # Добавляем OpenGL для QOpenGLWidget
)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/UI)

# Исполняемый файл
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME}
    glfw
    glad
    assimp
    stb_image
    ${GLFW_LIBRARIES}
    ${ASSIMP_LIBRARIES}
    OpenGL::GL
    Vulkan::Vulkan
    ui
)

# Специфичные настройки для Windows (DirectX)
if (WIN32)
    add_definitions(-D_WIN32)
    target_sources(${PROJECT_NAME} PRIVATE src/UI/direct3d_renderer.cpp)  # Исправлено на ${PROJECT_NAME}
    target_link_libraries(${PROJECT_NAME} PRIVATE d3d11 dxgi d3dcompiler)
endif()