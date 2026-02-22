cmake_minimum_required(VERSION 3.20)
project(jp_engine_render C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── 出力ディレクトリ ────────────────────────────────────
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ── はじむ インクルードパス ─────────────────────────────
if(NOT DEFINED HAJIMU_INCLUDE_DIR)
    foreach(P "../../jp/include" "../jp/include" "/usr/local/include/hajimu")
        if(EXISTS "${P}/hajimu_plugin.h")
            set(HAJIMU_INCLUDE_DIR "${P}")
            break()
        endif()
    endforeach()
endif()
if(NOT EXISTS "${HAJIMU_INCLUDE_DIR}/hajimu_plugin.h")
    message(FATAL_ERROR "hajimu_plugin.h が見つかりません。HAJIMU_INCLUDE_DIR を指定してください。")
endif()
message(STATUS "HAJIMU_INCLUDE_DIR = ${HAJIMU_INCLUDE_DIR}")

# ── SDL2 ────────────────────────────────────────────────
find_package(SDL2 REQUIRED)
message(STATUS "SDL2 found: ${SDL2_INCLUDE_DIRS}")

# ── ソースファイル ──────────────────────────────────────
set(ENG_RENDER_SOURCES
    src/eng_window.c
    src/eng_shader.c
    src/eng_texture.c
    src/eng_batch.c
    src/eng_camera.c
    src/eng_font.c
    src/plugin.c
)

# ── ターゲット ──────────────────────────────────────────
add_library(engine_render SHARED ${ENG_RENDER_SOURCES})

# 出力名: engine_render.hjp
set_target_properties(engine_render PROPERTIES
    PREFIX     ""
    SUFFIX     ".hjp"
    OUTPUT_NAME "engine_render"
)

target_include_directories(engine_render PRIVATE
    ${HAJIMU_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/src
)

# ── macOS OpenGL ────────────────────────────────────────
if(APPLE)
    target_compile_definitions(engine_render PRIVATE GL_SILENCE_DEPRECATION)
    target_link_libraries(engine_render PRIVATE
        ${SDL2_LIBRARIES}
        "-framework OpenGL"
        "-framework CoreFoundation"
    )
elseif(UNIX)
    target_link_libraries(engine_render PRIVATE
        ${SDL2_LIBRARIES}
        GL
        m
    )
elseif(WIN32)
    target_link_libraries(engine_render PRIVATE
        ${SDL2_LIBRARIES}
        opengl32
    )
endif()

# ── コンパイルオプション ────────────────────────────────
target_compile_options(engine_render PRIVATE
    -O2 -Wall -Wextra -Wno-unused-parameter
)
