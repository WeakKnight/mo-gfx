cmake_minimum_required(VERSION 3.1)
project(mo-gfx-samples)

set(
	BIN_NAME
	"mo-gfx-samples"
)

# Set C++11
set (CMAKE_CXX_STANDARD 11)

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG $<1:${CMAKE_SOURCE_DIR}/bin>)

include_directories(
    include/
    ../thirdparty/glfw/include/
	../include/
	thirdparty/glm/
	thirdparty/spdlog/include/
)

# Spdlog Building
add_subdirectory(thirdparty/spdlog)

# GLM Building
add_subdirectory(thirdparty/glm)

file(GLOB HEADERS "include/*.h")
file(GLOB SOURCES "src/*.cpp")

add_executable(${BIN_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${BIN_NAME} spdlog glfw ${GLFW_LIBRARIES} glm mo-gfx)