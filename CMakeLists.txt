cmake_minimum_required(VERSION 3.15)
project(Engine_BuildInfo_Win64_retail LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(MINHOOK_SRC
    src/minhook/src/buffer.c
    src/minhook/src/hook.c
    src/minhook/src/trampoline.c
    src/minhook/src/hde/hde64.c
)

add_library(Engine.BuildInfo_Win64_retail SHARED
    src/shim.c
    src/hook.c
    ${MINHOOK_SRC}
)

target_include_directories(Engine.BuildInfo_Win64_retail PRIVATE
    src
    src/minhook/include
    src/minhook/src/hde
)

target_link_libraries(Engine.BuildInfo_Win64_retail PRIVATE
    ws2_32
)

# Keep the output name exactly Engine.BuildInfo_Win64_retail.dll without prefix 'lib'
set_target_properties(Engine.BuildInfo_Win64_retail PROPERTIES
    OUTPUT_NAME "Engine.BuildInfo_Win64_retail"
    PREFIX ""
)

if (MINGW)
    target_link_options(Engine.BuildInfo_Win64_retail PRIVATE
        -static -static-libgcc
    )
endif()

