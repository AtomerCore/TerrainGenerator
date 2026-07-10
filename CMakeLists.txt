cmake_minimum_required(VERSION 4.1)
project(TerrainGenerator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================================
# 1. 头文件搜索路径
#    - include/                 : 自有头文件入口（PerlinNoise.h 等）
#    - include/imgui            : ImGui 核心头（其 backend 源码以 #include "imgui.h" 裸引用）
#    - include/imgui/backends   : ImGui backend 头（imgui_impl_*.h）
# ==========================================================
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/imgui
        ${PROJECT_SOURCE_DIR}/include/imgui/backends
)

# ==========================================================
# 2. 收集源文件
#    - src/            : 业务/工具/表现/编排层（自有源码）
#    - include/imgui/  : 第三方 ImGui 源码（按需求文档置于 include/imgui/）
# ==========================================================
file(GLOB IMGUI_SOURCES
        "include/imgui/*.cpp"
        "include/imgui/backends/imgui_impl_glfw.cpp"
        "include/imgui/backends/imgui_impl_opengl3.cpp"
)

set(APP_SOURCES
        src/main.cpp
        src/PerlinNoise.cpp
        src/TerrainGenerator.cpp
        src/Exporter.cpp
        src/UIStyle.cpp
)

# ==========================================================
# 3. 定义可执行文件
# ==========================================================
add_executable(${PROJECT_NAME}
        ${APP_SOURCES}
        ${IMGUI_SOURCES}
)

# ==========================================================
# 4. 链接库文件 (lib/)
#    使用静态 GLEW (glew32s.lib)，配合 GLEW_STATIC 宏，
#    无需运行期依赖 glew32.dll，避免缺失 DLL 导致构建/运行失败。
# ==========================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)

target_link_libraries(${PROJECT_NAME} PRIVATE
        # 1. GLEW 静态库 (glew32s.lib，配合 GLEW_STATIC 宏)
        ${PROJECT_SOURCE_DIR}/lib/glew32s.lib

        # 2. GLFW 静态库
        ${PROJECT_SOURCE_DIR}/lib/libglfw3.a

        # 3. Windows OpenGL 系统库
        opengl32
        glu32

        # 4. Windows 通用对话框库（GetSaveFileNameW / FR-05 保存路径选择）
        comdlg32
)

# 静态链接 GCC/C++ 运行时，便于单文件分发
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
