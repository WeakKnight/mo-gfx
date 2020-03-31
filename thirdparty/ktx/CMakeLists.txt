cmake_minimum_required(VERSION 2.8)

project(ktx LANGUAGES C CXX)

# libktx

message(STATUS "Adding libktx")

set(KTX_SOURCES
    lib/texture.c
    lib/hashlist.c
    lib/checkheader.c
    lib/swap.c
    lib/memstream.c
    lib/filestream.c
    lib/vkloader.c
    lib/vk_funcs.c
    lib/vk_funclist.inl
)
set(KTX_INCLUDE
    include
    lib
    other_include
)

add_library(ktx ${KTX_SOURCES})
target_include_directories(ktx PUBLIC ${KTX_INCLUDE})
target_include_directories(ktx PRIVATE Vulkan::Vulkan)
target_link_libraries(ktx Vulkan::Vulkan)
