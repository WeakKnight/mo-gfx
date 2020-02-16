project(imgui)

include_directories(./)
file(GLOB SOURCES ./*.cpp)

add_library(imgui ${SOURCES})