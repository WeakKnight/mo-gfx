cmake_minimum_required(VERSION 3.1)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(mo-gfx-samples)

# Set C++11
set (CMAKE_CXX_STANDARD 11)

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG $<1:${CMAKE_SOURCE_DIR}/bin>)

include_directories(
    ./include/
    ../thirdparty/glfw/include/
	../include/
	thirdparty/glm/
	thirdparty/spdlog/include/
)

# Spdlog Building
add_subdirectory(thirdparty/spdlog)

# GLM Building
add_subdirectory(thirdparty/glm)

#Multipass Example
file(GLOB MULTIPASS_HEADERS "./include/*.h" "./multi-pass/*.h")
file(GLOB MULTIPASS_SOURCES "./src/*.cpp" "./multi-pass/*.cpp")

add_executable("multi-pass" ${MULTIPASS_SOURCES} ${MULTIPASS_HEADERS})
target_link_libraries("multi-pass" spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx)

set_property(TARGET "multi-pass" PROPERTY FOLDER "mo-gfx-samples")

#ModelViewer Example
file(GLOB MODELVIEWER_HEADERS "./include/*.h" "./model-viewer/*.h")
file(GLOB MODELVIEWER_SOURCES "./src/*.cpp" "./model-viewer/*.cpp")

add_executable("model-viewer" ${MODELVIEWER_SOURCES} ${MODELVIEWER_HEADERS})
target_link_libraries("model-viewer" spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx)

set_property(TARGET "model-viewer" PROPERTY FOLDER "mo-gfx-samples")



