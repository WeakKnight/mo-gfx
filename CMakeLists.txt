cmake_minimum_required(VERSION 3.1)
project(mo-gfx VERSION 0.1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(
	LIB_NAME
	"mo-gfx"
)

# Set C++11
set (CMAKE_CXX_STANDARD 11)

include_directories(
    include/
    thirdparty/vma/
    thirdparty/glfw/include/
    thirdparty/shaderc/libshaderc/include/
)

# GLFW Building
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
add_subdirectory(thirdparty/glfw)

# Shaderc Building
set(SHADERC_SKIP_TESTS ON)
add_subdirectory(thirdparty/shaderc)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.cpp")

add_library(${LIB_NAME} ${SOURCES})
# add_executable(${LIB_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${LIB_NAME} glfw ${GLFW_LIBRARIES} shaderc)

find_package(Vulkan REQUIRED)

IF (WIN32)
target_compile_definitions(${LIB_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
ENDIF()

target_include_directories(${LIB_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${LIB_NAME} Vulkan::Vulkan)