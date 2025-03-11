cmake_minimum_required(VERSION 3.10)
project(SpecterEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)  # Для ресурсов
set(CMAKE_AUTOUIC ON)

# Поиск Qt
find_package(Qt5 COMPONENTS Widgets REQUIRED)

# Ресурсы
qt5_add_resources(RESOURCES resources.qrc)

# UI
file(GLOB UI_SRC "src/UI/*.cpp")
add_library(ui STATIC ${UI_SRC})
target_link_libraries(ui Qt5::Widgets)

# Исполняемый файл
add_executable(${PROJECT_NAME} src/main.cpp ${RESOURCES})
target_link_libraries(${PROJECT_NAME} ui)