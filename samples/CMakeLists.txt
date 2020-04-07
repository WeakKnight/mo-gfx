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
	thirdparty/assimp/include/
	../thirdparty/ktx/include/
)

# Spdlog Building
add_subdirectory(thirdparty/spdlog)

# GLM Building
add_subdirectory(thirdparty/glm)

# ASSIMP Building
add_subdirectory(thirdparty/assimp)

#Multipass Example
file(GLOB MULTIPASS_HEADERS "./include/*.h" "./multi-pass/*.h")
file(GLOB MULTIPASS_SOURCES "./src/*.cpp" "./multi-pass/*.cpp")

add_executable("multi-pass" ${MULTIPASS_SOURCES} ${MULTIPASS_HEADERS})
target_link_libraries("multi-pass" spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx assimp)

set_property(TARGET "multi-pass" PROPERTY FOLDER "mo-gfx-samples")

#ModelViewer Example
file(GLOB MODELVIEWER_HEADERS "./include/*.h" "./model-viewer/*.h")
file(GLOB MODELVIEWER_SOURCES "./src/*.cpp" "./model-viewer/*.cpp")

add_executable("model-viewer" ${MODELVIEWER_SOURCES} ${MODELVIEWER_HEADERS})
target_link_libraries("model-viewer" spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx assimp)

set_property(TARGET "model-viewer" PROPERTY FOLDER "mo-gfx-samples")

#Screeb Space Reflection Example
file(GLOB SSR_HEADERS "./include/*.h" "./screen-space-reflection/*.h")
file(GLOB SSR_SOURCES "./src/*.cpp" "./screen-space-reflection/*.cpp")

add_executable("screen-space-reflection" ${SSR_SOURCES} ${SSR_HEADERS})
target_link_libraries("screen-space-reflection" spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx assimp)

set_property(TARGET "screen-space-reflection" PROPERTY FOLDER "mo-gfx-samples")

#Raytracing Example
file(GLOB RT_HEADERS "./include/*.h" "./raytracing/*.h")
file(GLOB RT_SOURCES "./src/*.cpp" "./raytracing/*.cpp")

add_executable("raytracing" ${RT_SOURCES} ${RT_HEADERS})
target_link_libraries("raytracing" spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx assimp)

set_property(TARGET "raytracing" PROPERTY FOLDER "mo-gfx-samples")



