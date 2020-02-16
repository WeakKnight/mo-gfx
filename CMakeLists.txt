cmake_minimum_required(VERSION 3.1)
project(mo-vulkan-renderer VERSION 0.0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(
	BIN_NAME
	"mo-vulkan-renderer"
)

# Set C++11
set (CMAKE_CXX_STANDARD 11)

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG $<1:${CMAKE_SOURCE_DIR}/bin>)

include_directories(
    include/
    thirdparty/spdlog/include/
    thirdparty/imgui/
    thirdparty/vma/
    thirdparty/glfw/include/
    thirdparty/glm/
    thirdparty/tinygltf/
    thirdparty/shaderc/libshaderc/include/
)

# Spdlog Building
add_subdirectory(thirdparty/spdlog)

# GLFW Building
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(thirdparty/glfw)

# IMGUI Building
add_subdirectory(thirdparty/imgui)

# GLM Building
add_subdirectory(thirdparty/glm)

# TinyGLTF Building
add_subdirectory(thirdparty/tinygltf)

# Shaderc Building
set(SHADERC_SKIP_TESTS ON)
add_subdirectory(thirdparty/shaderc)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.cpp")

add_executable(${BIN_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${BIN_NAME} spdlog glfw ${GLFW_LIBRARIES} imgui glm shaderc)

find_package(Vulkan REQUIRED)

IF (WIN32)
target_compile_definitions(${BIN_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
ENDIF()

target_include_directories(${BIN_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${BIN_NAME} Vulkan::Vulkan)