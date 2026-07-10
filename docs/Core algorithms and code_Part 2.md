# main / Exporter / UIStyle 模块核心算法与编码


## 一、main.cpp

### 1.1 createTextureFromData() 纹理对象创建算法

OpenGL 2D 纹理的生成与初始化。把 `TerrainGenerator::getColorMap()` 返回的 RGB 字节流上传到 GPU，生成 1 个绑定名 `texture`，供主循环中 `glBindTexture` 反复使用。

```
┌────────────── createTextureFromData(data, width, height) ─────────────┐
│                                                                       │
│  ┌──── glGenTextures(1, &texture) ────┐                               │
│  │        申请 1 个纹理对象 ID           │                               │
│  └────────────────────┬───────────────┘                               │
│                       ▼                                               │
│  ┌────────── glBindTexture(GL_TEXTURE_2D, texture) ──────────┐        │
│  │           绑定到 2D 纹理目标                                 │        │
│  └────────────────────┬──────────────────────────────────────┘        │
│                       ▼                                               │
│  ┌───── 设置过滤方式（线性插值） ───────────────────────────┐              │
│  │  glTexParameteri(..., GL_TEXTURE_MIN_FILTER, LINEAR)│              │
│  │  glTexParameteri(..., GL_TEXTURE_MAG_FILTER, LINEAR)│              │
│  └────────────────────┬────────────────────────────────┘              │
│                       ▼                                               │
│  ┌────── glTexImage2D(..., GL_RGB, w, h, GL_UNSIGNED_BYTE, data) ──┐  │
│  │      上传整张图、分配 GPU 显存、生成 mipmap=0                        │  │
│  └────────────────────┬────────────────────────────────────────────┘  │
│                       ▼                                               │
│  ┌─────────────────────┐                                              │
│  │  return texture;    │                                              │
│  └─────────────────────┘                                              │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
```

### 1.2 updateTexture() 纹理原地更新算法

地形尺寸不变时，**复用** 原纹理对象，仅替换像素数据。避免反复 `glDeleteTextures + glGenTextures` 带来的 GPU 资源抖动。

```
┌────────────── updateTexture(texture, data, width, height) ─────────────┐
│                                                                        │
│  ┌────────── glBindTexture(GL_TEXTURE_2D, texture) ───────────┐        │
│  │           重新绑定同一纹理对象                                 │        │
│  └────────────────────┬───────────────────────────────────────┘        │
│                       ▼                                                │
│  ┌── glTexSubImage2D(..., 0, 0, w, h, GL_RGB, UBYTE, data) ─────┐      │
│  │      从 (0,0) 起整块覆盖像素，保留原纹理格式/参数                    │     │
│  └────────────────────┬─────────────────────────────────────────┘      │
│                       ▼                                                │
│  ┌─────────────────┐                                                   │
│  │     return;     │                                                   │
│  └─────────────────┘                                                   │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

### 1.3 等比绘制尺寸计算算法

根据视口宽高比与纹理宽高比，自适应计算纹理四边形的 `drawW / drawH`，**保证地形不拉伸**。`kBaseDrawSize = 1.8f` 是 OpenGL 归一化坐标下的基础半边长。

```
┌───────── computeDrawSize(viewW, viewH, texAspect, zoom) ─────────┐
│                                                                  │
│  ┌── aspect = viewW / viewH ─────────────────────────┐           │
│  │  texAspect = texW / texH                          │           │
│  └───────────────────────────────────────────────────┘           │
│                                                                  │
│  ┌────── aspect > texAspect ? ──────┐                            │
│  │   T ─► drawH = base / zoom       │                            │
│  │        drawW = drawH * texAspect │                            │
│  │   F ─► drawW = base / zoom       │                            │
│  │        drawH = drawW / texAspect │                            │
│  └─────────────────┬───────────────┘                             │
│                    ▼                                             │
│  ┌──────────────────────┐                                        │
│  │ return (drawW,drawH) │                                        │
│  └──────────────────────┘                                        │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 1.4 鼠标滚轮缩放 + 拖拽平移算法

在主循环中按帧检测：仅当鼠标落在右侧视口内、且未悬停在 ImGui 控件上时，启用平移与缩放。`viewOffsetX/Y` 累计位移，`viewZoom` 按 0.5~5.0 区间夹紧。

```
┌────────────── 鼠标交互 (主循环每帧) ───────────────────────────┐
│                                                            │
│  ┌────── 视口命中检测 ────────┐                               │
│  │  mouseInViewport =       │                              │
│  │    mousePos.x∈[viewX,    │                              │
│  │             viewX+viewW) │                              │
│  │    AND                   │                              │
│  │    mousePos.y∈[0, viewH) │                              │
│  └────────────┬─────────────┘                              │
│               ▼                                            │
│  ┌── enablePan ∧ mouseInViewport ∧ ¬IsAnyItemHovered ? ──┐ │
│  │   F ─► 跳过本帧所有鼠标事件                               │ │
│  │   T ─► 继续                                            │ │
│  └────────────────┬──────────────────────────────────────┘ │
│                    ▼                                       │
│  ┌────── wheel = GetIO().MouseWheel ───────┐               │
│  │   wheel != 0 ?                          │               │
│  │     T ─► viewZoom = clamp(              │               │
│  │            viewZoom - wheel*0.1,        │               │
│  │            0.5, 5.0)                    │               │
│  └─────────────────────────────────────────┘               │
│                                                            │
│  ┌────── IsMouseDragging(Left) ? ──────┐                   │
│  │   T ─► delta = GetMouseDragDelta    │                   │
│  │        viewOffsetX +=               │                   │
│  │            delta.x*(2*drawW/viewW)  │                   │
│  │        viewOffsetY -=               │                   │
│  │            delta.y*(2*drawH/viewH)  │                   │
│  │        ResetMouseDragDelta(Left)    │                   │
│  └─────────────────────────────────────┘                   │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### 1.5 主循环 + 按需重生成算法

`needsRegen` 标志位贯穿 UI 与渲染两个阶段：UI 控件变化置位 → 渲染阶段在 `view.autoGenerate==true` 时执行生成 → 比较新旧尺寸决定 `create` 或 `update` 纹理。

```
┌────────────────────── 主循环 while (!ShouldClose) ───────────────────────┐
│                                                                         │
│  ┌───── 帧头 ───────┐                                                    │
│  │  glfwPollEvents │                                                    │
│  │  ImGui NewFrame │                                                    │
│  └──────┬──────────┘                                                    │
│         ▼                                                               │
│  ┌── 绘制左侧控制面板 (DrawControlPanel) ─────────────────────┐            │
│  │  • 控件修改 → ctx.needsRegen = true                      │             │
│  │  • 导出按钮 → ctx.exportHeightmapRequested               │             │
│  │  • 导出按钮 → ctx.exportColormapRequested                │             │
│  └──────┬──────────────────────────────────────────────────┘            │
│         ▼                                                               │
│  ┌────── 导出请求处理 ──────────────────────────┐                          │
│  │  if heightmapRequested:                   │                          │
│  │     path = pickSavePath()                 │                          │
│  │     if path: exportHeightmap(path, ...)   │                          │
│  │  if colormapRequested:                    │                          │
│  │     path = pickSavePath()                 │                          │
│  │     if path: exportColormap(path, ...)    │                          │
│  └──────┬────────────────────────────────────┘                          │
│         ▼                                                               │
│  ┌────── 条件生成 ──────────────────┐                                     │
│  │  if needsRegen &&              │                                     │
│  │     view.autoGenerate          │                                     │
│  │   ┌──────────────────┐         │                                     │
│  │   │ generator.       │         │                                     │
│  │   │   generate(      │         │                                     │
│  │   │     params,      │         │                                     │
│  │   │     displayMode) │         │                                     │
│  │   └────────┬─────────┘         │                                     │
│  │            ▼                   │                                     │
│  │   ┌──────────────────┐         │                                     │
│  │   │ newW,newH =      │         │                                     │
│  │   │   getW/getH      │         │                                     │
│  │   └────────┬─────────┘         │                                     │
│  │            ▼                   │                                     │
│  │   ┌── newW!=W ∨ newH!=H ? ──┐  │                                     │
│  │   │ T ─► glDelete+create    │  │                                     │
│  │   │ F ─► updateTexture      │  │                                     │
│  │   └────────┬────────────────┘  │                                     │
│  │            ▼                   │                                     │
│  │   ┌──────────────────┐         │                                     │
│  │   │  refreshStats()  │         │                                     │
│  │   └────────┬─────────┘         │                                     │
│  │            ▼                   │                                     │
│  │   ┌──────────────────┐         │                                     │
│  │   │ needsRegen=false │         │                                     │
│  │   └──────────────────┘         │                                     │
│  └──────┬─────────────────────────┘                                     │
│         ▼                                                               │
│  ┌──── 右侧视口渲染 ─────────┐                                             │
│  │  glViewport(viewX,0,   │                                             │
│  │           viewW,viewH) │                                             │
│  │  glClear(...)          │                                             │
│  │  glEnable(TEXTURE_2D)  │                                             │
│  │  glBindTexture(...)    │                                             │
│  │  drawW,drawH =         │                                             │
│  │    computeDrawSize(...)│                                             │
│  │  鼠标交互更新             │                                             │
│  │    viewZoom,offset     │                                             │
│  │  glBegin(GL_QUADS)     │                                             │
│  │    4 顶点 + 4 纹理坐标   │                                             │
│  │  glEnd / glDisable     │                                             │
│  │  DrawViewportOverlays  │  ←  图例 + 统计                               │
│  └──────┬─────────────────┘                                             │
│         ▼                                                               │
│  ┌──── 帧尾 ────────────────────────────┐                                │
│  │  ImGui::Render                      │                                │
│  │  ImGui_ImplOpenGL3_RenderDrawData   │                                │
│  │  glfwSwapBuffers                    │                                │
│  └─────────────────────────────────────┘                                │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.6 完整编码

```cpp
// ============================================================
// main.cpp
// 实现功能：DR-03（OpenGL 纹理对象）、FR-04（交互控制）、FR-06（UI/UX）
// ============================================================

#include <iostream>
#include <cstdio>
#include <cstdint>
#include <string>
#include <algorithm>

// 导入ImGui库函数
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

// 导入GLEW/GLFW库函数
#include "GL/glew.h"
#include "GLFW/glfw3.h"

// 导入功能模块
#include "TerrainGenerator.h"
#include "Exporter.h"
#include "UIStyle.h"


// DR-03: OpenGL 纹理对象
// 视口内纹理四边形的基础半边长
static constexpr float kBaseDrawSize = 1.8f;

// 初始化创建纹理
static GLuint createTextureFromData(const std::vector<unsigned char>& data, int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return texture;
}

// 刷新纹理
static void updateTexture(GLuint texture, const std::vector<unsigned char>& data, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.data());
}



int main() {

    // 初始化GLFW着色器设置
    glfwInit();

    // 设定为OpenGL 3.0版本
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(1400, 900, "2D Terrain Generator", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 初始化GLEW
    glewInit();

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // FR-06-03: 应用自定义UI风格
    UIStyle::ApplyCustomImGuiStyle();

    // 绑定ImGui和OpenGL窗口
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 初始化地形生成
    TerrainParams params;                         // 地形参数
    ViewState view;                               // 显示参数
    TerrainGenerator generator;                   // 创建generator对象
    generator.generate(params, view.displayMode); // 生成地形

    // 导入OpenGL纹理
    GLuint terrainTexture = createTextureFromData(
        generator.getColorMap(),
        generator.getWidth(),
        generator.getHeight()
    );
    int terrainTexW = generator.getWidth();
    int terrainTexH = generator.getHeight();


    // FR-03-02: Show Stats缓存，仅在改变地形参数时重新生成
    float statMin = 0.0f, statMax = 0.0f, statMean = 0.0f, statLandRatio = 0.0f;
    bool statsValid = false;

    auto refreshStats = [&]() {
        TerrainStats s = TerrainGenerator::computeStats(generator.getHeightMap(), params.seaLevel);
        statMin = s.min;
        statMax = s.max;
        statMean = s.mean;
        statLandRatio = s.landRatio;
        statsValid = s.valid;
    };
    refreshStats();



    // 状态标记是否为最新地形
    bool needsRegen = false;

    // Terrain Seed: 地形唯一配置标识
    char seedStr[32] = {0};
    {
        uint64_t initSeed = encodeTerrainSeed(params);
        std::snprintf(seedStr, sizeof(seedStr), "%016llX",  /* 016: 固定16个字符 , llx:固定大写16进制 */
                      static_cast<unsigned long long>(initSeed));
    }

    // 导出默认路径设置
    char exportPath[256] = "terrain_heightmap.pgm";
    char exportColorPath[256] = "terrain_colormap.ppm";


    // ====================== 主循环 ======================
    while (!glfwWindowShouldClose(window)) {

        // 轮询操作系统所有窗口输入事件
        glfwPollEvents();

        // NewFrame: OpenGL -> GLFW -> ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 获取窗口尺寸
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // FR-06-01: 左侧面板自适应宽度
        float panelWidth = std::clamp(display_w * 0.32f, 480.0f, 640.0f);

        // 设置左侧面板固定，不可移动
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(panelWidth, static_cast<float>(display_h)), ImGuiCond_Always);


        // 控制面板控件渲染
        UIStyle::ControlPanelContext ctx{
            params,
            view,
            seedStr,         static_cast<int>(sizeof(seedStr)),
            exportPath,      static_cast<int>(sizeof(exportPath)),
            exportColorPath, static_cast<int>(sizeof(exportColorPath)),
            needsRegen
        };
        UIStyle::DrawControlPanel(ctx, panelWidth, io.Framerate);


        // FR-05: 导出功能
        if (ctx.exportHeightmapRequested) {
            // FR-05-01: 导出为PGM
            std::string chosen = Exporter::pickSavePath(false, exportPath);
            if (!chosen.empty()) {
                std::snprintf(exportPath, sizeof(exportPath), "%s", chosen.c_str());
                Exporter::exportHeightmap(exportPath, generator.getHeightMap(),
                                              generator.getWidth(), generator.getHeight());

            }
            ctx.exportHeightmapRequested = false;
        }
        if (ctx.exportColormapRequested) {
            // FR-05-02: 导出为PPM
            std::string chosen = Exporter::pickSavePath(true, exportColorPath);
            if (!chosen.empty()) {
                std::snprintf(exportColorPath, sizeof(exportColorPath), "%s", chosen.c_str());
                Exporter::exportColormap(exportColorPath, generator.getColorMap(),
                                             generator.getWidth(), generator.getHeight());

            }
            ctx.exportColormapRequested = false;
        }


        // FR-04-01: 控制是否自动重生成
        if (needsRegen && view.autoGenerate) {

            // 生成新地形图
            generator.generate(params, view.displayMode);

            // 获取新地形
            int newW = generator.getWidth();
            int newH = generator.getHeight();

            // 判断地形尺寸发生变化 -> 判断更新还是新建OpenGL纹理
            if (newW != terrainTexW || newH != terrainTexH) {
                glDeleteTextures(1, &terrainTexture);
                terrainTexture = createTextureFromData(generator.getColorMap(), newW, newH);
                terrainTexW = newW;
                terrainTexH = newH;
            } else {
                updateTexture(terrainTexture, generator.getColorMap(), newW, newH);
            }

            // FR-03-02: 更新缓存
            refreshStats();

            needsRegen = false;
        }

        // 右侧地形展示窗口
        float viewX = panelWidth;
        float viewW = display_w - panelWidth;
        float viewH = display_h;

        glViewport(static_cast<int>(viewX), 0, static_cast<int>(viewW), static_cast<int>(viewH));
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 开启纹理管线，并绑定地形贴图
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, terrainTexture);

        // 计算等比绘制尺寸，防止地形拉伸变形
        float zoom = view.viewZoom;
        float aspect = viewW / viewH;
        float texAspect = static_cast<float>(generator.getWidth()) / static_cast<float>(generator.getHeight());

        float drawW, drawH;
        if (aspect > texAspect) {
            drawH = kBaseDrawSize / zoom;
            drawW = drawH * texAspect;
        } else {
            drawW = kBaseDrawSize / zoom;
            drawH = drawW / texAspect;
        }


        // FR-06-04: 鼠标滑轮滚动/拖动操作
        ImVec2 mousePos = ImGui::GetMousePos();
        bool mouseInViewport = (mousePos.x >= viewX && mousePos.x < viewX + viewW &&
                                mousePos.y >= 0.0f   && mousePos.y < viewH);

        if (view.enablePan && mouseInViewport && !ImGui::IsAnyItemHovered()) {

            // 鼠标滚轮缩放
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f) {
                view.viewZoom = std::clamp(view.viewZoom - wheel * 0.1f, 0.5f, 5.0f);
            }

            // 鼠标拖拽平移
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

                // 坐标换算
                view.viewOffsetX += delta.x * (2.0f * drawW / viewW);
                view.viewOffsetY -= delta.y * (2.0f * drawH / viewH);

                ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
            }
        }

        // 地形图渲染与鼠标操作适配
        float offX = view.viewOffsetX;
        float offY = view.viewOffsetY;
        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex2f(-drawW + offX, -drawH + offY);
        glTexCoord2f(1, 1); glVertex2f( drawW + offX, -drawH + offY);
        glTexCoord2f(1, 0); glVertex2f( drawW + offX,  drawH + offY);
        glTexCoord2f(0, 0); glVertex2f(-drawW + offX,  drawH + offY);
        glEnd();
        glDisable(GL_TEXTURE_2D);


        // FR-03-02: 视口角落图例与统计小窗
        UIStyle::DrawViewportOverlays(params, view, viewX, viewW, viewH,
                                      statMin, statMax, statMean, statLandRatio,
                                      statsValid);


        // 统一渲染
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 交换缓冲区
        glfwSwapBuffers(window);
    }

    // 清理
    glDeleteTextures(1, &terrainTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
```




## 二、Exporter.{h,cpp}

### 2.1 exportHeightmap() PGM (P5) 灰度图导出算法

把 `std::vector<float>` 形式的 heightMap 一次性映射到 `[0, 255]` 字节流，按 PGM 二进制格式 (P5) 落盘。

```
┌─────────── exportHeightmap(filename, heightMap, w, h) ─────────────┐
│                                                                    │
│  ┌── if heightMap.empty() ───┐                                     │
│  │   T ─► return false       │                                     │
│  └────────────┬──────────────┘                                     │
│               ▼                                                    │
│  ┌────────── ofstream(filename, binary) ───────────┐               │
│  │     if !file ─► return false                    │               │
│  └────────────────────┬────────────────────────────┘               │
│                       ▼                                            │
│  ┌──── PGM 头（ASCII 行）──────────────────────┐                     │
│  │  "P5\n"                                    │                    │
│  │  width << " " << height << "\n"            │                    │
│  │  "255\n"                                   │                    │
│  └────────────────────┬───────────────────────┘                    │
│                       ▼                                            │
│  ┌── 批量转换 h*255 -> uchar ───┐                                    │
│  │   buffer.reserve(size)      │                                   │
│  │   for h in heightMap:       │                                   │
│  │     buffer.push_back(       │                                   │
│  │       uint8(h * 255))       │                                   │
│  └────────────────────┬────────┘                                   │
│                       ▼                                            │
│  ┌── file.write(buffer.data(), buffer.size()) ────┐                │
│  │       一次写入整块像素                            │                │
│  └────────────────────┬───────────────────────────┘                │
│                       ▼                                            │
│  ┌──────────────────┐                                              │
│  │  return true;    │                                              │
│  └──────────────────┘                                              │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

### 2.2 exportColormap() PPM (P6) 彩图导出算法


```
┌─────────── exportColormap(filename, colorMap, w, h) ──────────────┐
│                                                                   │
│  ┌── if colorMap.empty() ────┐                                    │
│  │   T ─► return false       │                                    │
│  └────────────┬──────────────┘                                    │
│               ▼                                                   │
│  ┌── ofstream(filename, binary) ──┐                               │
│  │   if !file ─► return false     │                               │
│  └────────────────────┬───────────┘                               │
│                       ▼                                           │
│  ┌──── PPM 头（ASCII 行）────────────────┐                          │
│  │  "P6\n"                              │                         │
│  │  width << " " << height << "\n"      │                         │
│  │  "255\n"                                                       │
│  └────────────────────┬─────────────────┘                         │
│                       ▼                                           │
│  ┌── file.write(colorMap.data(), colorMap.size()) ──┐             │
│  │       一次写入整块像素（零拷贝）                       │             │
│  └────────────────────┬─────────────────────────────┘             │
│                       ▼                                           │
│  ┌──────────────────┐                                             │
│  │  return true;    │                                             │
│  └──────────────────┘                                             │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

### 2.3 pickSavePath() Windows "另存为" 对话框算法

调用 Win32 `GetSaveFileNameW` 弹出原生另存为对话框。`isColormap`决定过滤器与默认扩展名 (`.ppm` / `.pgm`)

```
┌────────── pickSavePath(isColormap, currentPath) ───────────────────┐
│                                                                    │
│  ┌── 选过滤器 & 默认扩展名 ────────────────────┐                       │
│  │  filter = isColormap ? "*.ppm" : "*.pgm" │                      │
│  │  defExt = isColormap ? "ppm"   : "pgm"   │                      │
│  └────────────────────┬─────────────────────┘                      │
│                       ▼                                            │
│  ┌── UTF-8 → UTF-16，提取文件名部分 ─────────────┐                    │
│  │  wszFile = wcsdup of currentPath           │                    │
│  │  base = last of "\" or "/" + 1             │                    │
│  │  wcsncpy_s(wszFile, base)                  │                    │
│  └────────────────────┬───────────────────────┘                    │
│                       ▼                                            │
│  ┌─────── 填充 OPENFILENAMEW 并弹出对话框 ────────┐                   │
│  │  ofn.lStructSize = sizeof(ofn)              │                   │
│  │  ofn.lpstrFile   = wszFile                  │                   │
│  │  ofn.lpstrFilter = filter                   │                   │
│  │  ofn.lpstrDefExt = defExt                   │                   │
│  │  ofn.Flags       =                          │                   │
│  │    OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST  │                   │
│  │  GetSaveFileNameW(&ofn)                     │                   │
│  └────────────────────┬────────────────────────┘                   │
│                       ▼                                            │
│  ┌── GetSaveFileNameW 返回 false ? ───┐                             │
│  │  T ─► return ""（用户取消）          │                             │
│  └────────────────────┬──────────────┘                             │
│                       ▼                                            │
│  ┌── UTF-16 → UTF-8，去掉末尾 \0 ───────────────┐                    │
│  │  ulen = WideCharToMultiByte(...)           │                    │
│  │  result.resize(ulen)                       │                    │
│  │  WideCharToMultiByte(..., result.data())   │                    │
│  │  if result.back() == '\0' result.pop_back()│                    │
│  └────────────────────┬───────────────────────┘                    │
│                       ▼                                            │
│  ┌──────────────────┐                                              │
│  │  return result;  │                                              │
│  └──────────────────┘                                              │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

### 2.4 完整编码

```cpp
// ============================================================
// Exporter.h
// 实现需求: FR-05（数据导出）、DR-06（导出文件数据格式）
// ============================================================


#pragma once

#include <string>
#include <vector>

namespace Exporter {

// 导出PGM（P5）二进制灰度图
bool exportHeightmap(const char* filename,
                     const std::vector<float>& heightMap,
                     int width, int height);

// 导出彩色图PPM（P6）二进制彩图
bool exportColormap(const char* filename,
                    const std::vector<unsigned char>& colorMap,
                    int width, int height);


// Windows原生"另存为"对话框
std::string pickSavePath(bool isColormap, const char* currentPath);

}
```

```cpp
// ============================================================
// Exporter.cpp
// 实现需求: FR-05（数据导出）、DR-06（导出文件数据格式）
// ============================================================


#include "Exporter.h"

#include <fstream>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>


namespace Exporter {

// 导出Heightmap
bool exportHeightmap(const char* filename,
                     const std::vector<float>& heightMap,
                     int width, int height) {
    if (heightMap.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // PGM(P5)文件头
    file << "P5\n";
    file << width << " " << height << "\n";
    file << "255\n";

    // 批量转换并一次写入
    std::vector<unsigned char> buffer;
    buffer.reserve(heightMap.size());
    for (float h : heightMap) {
        buffer.push_back(static_cast<unsigned char>(h * 255));
    }
    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));

    return true;
}

// 导出Colormap
bool exportColormap(const char* filename,
                    const std::vector<unsigned char>& colorMap,
                    int width, int height) {
    if (colorMap.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // DR-06: PPM (P6) 文件头
    file << "P6\n";
    file << width << " " << height << "\n";
    file << "255\n";

    file.write(reinterpret_cast<const char*>(colorMap.data()), static_cast<std::streamsize>(colorMap.size()));
    return true;
}


// 弹出Windows原生"另存为"对话框
std::string pickSavePath(bool isColormap, const char* currentPath) {

    // 过滤器字符串：双 \0 结尾；数组形式保留嵌入的 \0
    static const wchar_t filterPgm[] = L"PGM Image (*.pgm)\0*.pgm\0All Files (*.*)\0*.*\0";
    static const wchar_t filterPpm[] = L"PPM Image (*.ppm)\0*.ppm\0All Files (*.*)\0*.*\0";

    // 把当前路径（UTF-8）转 UTF-16，取出文件名部分作为对话框初始文件名
    wchar_t wszFile[MAX_PATH] = {0};
    if (currentPath && *currentPath) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, currentPath, -1, nullptr, 0);
        std::wstring wCurrent(static_cast<size_t>(wlen), 0);
        MultiByteToWideChar(CP_UTF8, 0, currentPath, -1, wCurrent.data(), wlen);

        const wchar_t* base = wCurrent.c_str();
        const wchar_t* slash  = wcsrchr(wCurrent.c_str(), L'\\');
        const wchar_t* slash2 = wcsrchr(wCurrent.c_str(), L'/');
        // 修复原实现的 nullptr 比较 UB：仅当 slash2 非空且（slash 为空或 slash2 更靠后）时采用 slash2
        if (slash2 && (!slash || slash2 > slash)) slash = slash2;
        if (slash) base = slash + 1;
        wcsncpy_s(wszFile, MAX_PATH, base, _TRUNCATE);
    }

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFile       = wszFile;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFilter     = isColormap ? filterPpm : filterPgm;
    ofn.nFilterIndex    = 1;
    ofn.lpstrDefExt     = isColormap ? L"ppm" : L"pgm";
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    // 用户取消或出错
    if (!GetSaveFileNameW(&ofn)) {
        return std::string();
    }

    // 把选中的宽字符路径转回UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wszFile, -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(ulen), 0);
    WideCharToMultiByte(CP_UTF8, 0, wszFile, -1, result.data(), ulen, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') result.pop_back();   // 去掉末尾 \0
    return result;
}

}
```




## 三、UIStyle.{h,cpp}

### 3.1 ApplyCustomImGuiStyle() 自定义主题算法

在 ImGui 上下文创建后调用一次：先加载 JetBrainsMono 字体作为默认字体；再统一设置圆角、间距、边框尺寸；最后通过一个 `Col` 数组把 `ImGuiCol_*` 全部映射为自定义UI参数，覆盖背景/边框/文字/Frame/标题栏/滚动条/按钮/Header/标签 等 30+ 种颜色。

```
┌────────────────── ApplyCustomImGuiStyle() ──────────────────────┐
│                                                                 │
│  ┌── 加载 JetBrainsMono 字体 ──────────────────────────┐          │
│  │  io.Fonts->Clear()                                │          │
│  │  font = AddFontFromFileTTF(                       │          │
│  │        "../include/ttf/JetBrainsMono-Regular.ttf",│          │
│  │        16.0f)                                     │          │
│  │  io.FontDefault = font ?: AddFontDefault()        │          │
│  └─────────────────────┬─────────────────────────────┘          │
│                        ▼                                        │
│  ┌── 圆角 / 间距 / 边框 / 最小尺寸 ──────────────────────┐          │
│  │  WindowRounding  = 12   ChildRounding  = 10       │          │
│  │  FrameRounding   =  8   PopupRounding  = 10       │          │
│  │  ScrollbarRounding=10  GrabRounding   =  6        │          │
│  │  TabRounding     =  8                             │          │
│  │  WindowPadding   = (14,14)  FramePadding=(8,5)    │          │
│  │  ItemSpacing     = (8, 8)   ItemInnerSpacing=(8,4)│          │
│  │  IndentSpacing   = 22  ScrollbarSize=12           │          │
│  │  WindowBorderSize= 1   ChildBorderSize=1          │          │
│  │  WindowMinSize   = (320, 200)                     │          │
│  └─────────────────────┬─────────────────────────────┘          │
│                        ▼                                        │
│  ┌──── Col 数组（30+ 项）覆盖 ImGuiCol_* 颜色 ────────────────┐     │
│  │  {ImGuiCol_WindowBg,        rgba(0.10,0.10,0.14,0.55)} │     │
│  │  {ImGuiCol_ChildBg,         rgba(0.12,0.12,0.18,0.40)} │     │
│  │  {ImGuiCol_Text,            rgba(0.95,0.95,0.98,1.00)} │     │
│  │  {ImGuiCol_Button,          rgba(0.22,0.28,0.40,0.70)} │     │
│  │  {ImGuiCol_ButtonHovered,   rgba(0.35,0.50,0.80,0.85)} │     │
│  │  {ImGuiCol_CheckMark,       rgba(0.45,0.65,1.00,1.00)} │     │
│  │  ... (Title/Scrollbar/Header/Separator/Tab/ResizeGrip) │     │
│  └─────────────────────┬──────────────────────────────────┘     │
│                        ▼                                        │
│  ┌── for e in cols: s.Colors[e.idx] = e.col ──┐                 │
│  │     一遍循环写完所有色                         │                 │
│  └─────────────────────┬──────────────────────┘                 │
│                        ▼                                        │
│  ┌──────────────────┐                                           │
│  │     return;      │                                           │
│  └──────────────────┘                                           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 DrawControlPanel() 控制面板渲染算法

按 FR-01 ~ FR-06 顺序，把面板垂直拆分为若干分区（噪声/Seed/形态/显示/控制/预设/导出），每分区调用`DrawXxxSection`子函数。

```
┌────────────── DrawControlPanel(ctx, panelWidth, framerate) ───────────────┐
│                                                                           │
│  ┌── 固定窗口 (NoMove/NoResize/NoCollapse/NoTitleBar) ─────┐                │
│  │  if !ImGui::Begin("Control Panel") return              │               │
│  └────────────────────┬───────────────────────────────────┘               │
│                       ▼                                                   │
│  ┌── 字体 + 按钮高度随面板宽度动态缩放 ────────────────────┐                    │
│  │  fontScale = clamp(panelWidth/480, 1.0, 1.6)      │                    │
│  │  SetWindowFontScale(fontScale)                    │                    │
│  │  layout = { panelWidth, fontScale,                │                    │
│  │             panelWidth-20*fontScale,              │                    │
│  │             28*fontScale }                        │                    │
│  └────────────────────┬──────────────────────────────┘                    │
│                       ▼                                                   │
│  ┌──── 标题 + 分隔线 ────┐                                                  │
│  │  "Terrain Generator" │                                                 │
│  │  Separator()         │                                                 │
│  └────────┬─────────────┘                                                 │
│           ▼                                                               │
│  ┌────── for each section (Separator between) ──────────────────────┐     │
│  │  DrawNoiseSection(ctx, terrainParamChanged)   // FR-01           │     │
│  │  DrawSeedSection(ctx, panelWidth)             // Seed + Random   │     │
│  │  DrawShapeSection(ctx, terrainParamChanged)   // FR-02           │     │
│  │  DrawDisplaySection(ctx, panelWidth)          // FR-03           │     │
│  │  DrawControlsSection(ctx, layout, ...)        // FR-04           │     │
│  │  DrawPresetsSection(ctx, layout, ...)         // FR-04-04        │     │
│  │  DrawExportSection(ctx, layout)               // FR-05           │     │
│  └────────┬─────────────────────────────────────────────────────────┘     │
│           ▼                                                               │
│  ┌── if terrainParamChanged ─────────────────────────────┐                │
│  │  seed = encodeTerrainSeed(params)                     │                │
│  │  snprintf(seedStr, ..., "%016llX", seed)              │                │
│  └────────────────────┬──────────────────────────────────┘                │
│                       ▼                                                   │
│  ┌── Separator + FPS 显示 ───┐                                             │
│  │  Separator()             │                                             │
│  │  Text("FPS: %.1f", fps)  │                                             │
│  └──────────────────────────┘                                             │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
```

### 3.3 DrawSeedSection() 种子解析与随机化算法

Seed 文本框按十六进制编辑；按回车提交时，先 `strtoull` 解析，再 `decodeTerrainSeed` 还原所有 `TerrainParams` 字段，再 `encode` 一次回写。
"Randomize" 按钮用 `mt19937 + uniform_int_distribution` 拼出 64-bit 随机种子并按同样流程回填。

```
┌────────────── DrawSeedSection(ctx, panelWidth) ──────────────────────┐
│                                                                      │
│  ┌── 文本框 (16 字符十六进制) ────────────────────────────────┐          │
│  │  if ImGui::InputText("##TerrainSeed",                  │          │
│  │                     seedStr, size,                     │          │
│  │                     HEX | EnterReturnsTrue) {          │          │
│  │     newSeed = strtoull(seedStr, nullptr, 16)           │          │
│  │     decodeTerrainSeed(newSeed, params)                 │          │
│  │     enc = encodeTerrainSeed(params)                    │          │
│  │     snprintf(seedStr, ..., "%016llX", enc)             │          │
│  │     ctx.needsRegen = true                              │          │
│  │  }                                                     │          │
│  └────────────────────┬───────────────────────────────────┘          │
│                       ▼                                              │
│  ┌── SameLine + Randomize 按钮 ───────────────────────────┐           │
│  │  if Button("Randomize", ...) {                        │           │
│  │     static mt19937 rng(random_device{}())             │           │
│  │     uniform_int_distribution<int> dist(0, 0xFFFF)     │           │
│  │     rSeed = 0                                         │           │
│  │     for i=0..3:                                       │           │
│  │        rSeed = (rSeed<<16) ^ dist(rng)                │           │
│  │     decodeTerrainSeed(rSeed, params)                  │           │
│  │     enc = encodeTerrainSeed(params)                   │           │
│  │     snprintf(seedStr, ..., "%016llX", enc)            │           │
│  │     ctx.needsRegen = true                             │           │
│  │  }                                                    │           │
│  └───────────────────────────────────────────────────────┘           │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

### 3.4 DrawPresetsSection() 4×2 预设按钮布局算法

```
┌────── DrawPresetsSection(ctx, layout, terrainParamChanged) ────────┐
│                                                                   │
│  ┌── halfW = layout.buttonWidth / 2 - 5 ──────────────┐           │
│  └────────────────────┬───────────────────────────────┘           │
│                       ▼                                           │
│  ┌── Row 1 ─────────────────────────────────────────────┐         │
│  │  Button("Smooth Plains",      halfW)                 │         │
│  │      → applyPresetSmoothPlains(params)               │         │
│  │  SameLine                                            │         │
│  │  Button("Rugged Mountains",   halfW)                 │         │
│  │      → applyPresetRuggedMountains(params)            │         │
│  └──────────────────────────────────────────────────────┘         │
│  ┌── Row 2 ─────────────────────────────────────────────┐         │
│  │  Button("Large Continent",     halfW)                │         │
│  │      → applyPresetLargeContinent(params)             │         │
│  │  SameLine                                            │         │
│  │  Button("Fragmented Isles",    halfW)                │         │
│  │      → applyPresetFragmentedIsles(params)            │         │
│  └──────────────────────────────────────────────────────┘         │
│                                                                   │
│  (任一按钮按下均设置 needsRegen=true, terrainParamChanged=true)       │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

### 3.5 DrawViewportOverlays() 视口图例与统计算法

依据 `view.showLegend` / `view.showStats` 两个开关，用 `ImGui::SetNextWindowPos + SetNextWindowBgAlpha(0.70f)` 把覆盖层固定在视口左下/右上角。

- **图例**：显示模式 0 → 列出 `BIOMES[0 ~ BIOME_COUNT-1]` 的 10 个色块；显示模式 1/2 → 画一条 16 段渐变条 (Grayscale 白→黑 / Heatmap 蓝→绿→红)。
- **统计**：从 `computeStats` 缓存里直接读出 min/max/mean/land%/size，文本输出。

```
┌──────── DrawViewportOverlays(params, view, viewX, viewW, viewH, ...) ─────┐
│                                                                           │
│  ┌── overlayFlags: NoMove|NoResize|NoCollapse|                            │
│  │              NoTitleBar|NoFocusOnAppearing|                            │
│  │              NoNav|NoBringToFrontOnFocus                               │
│  └────────────────────────────────────────────────────────────────────────┘
│                                                                           │
│  ┌── if view.showLegend ────────────────────────────────────────┐         │
│  │  SetNextWindowPos(viewX+pad, viewH-pad, pivot=(0,1))         │         │
│  │  SetNextWindowBgAlpha(0.70)                                  │         │
│  │  if Begin("##Legend", overlayFlags|AutoResize):              │         │
│  │     SetWindowFontScale(1.2)                                  │         │
│  │     if displayMode==0:                                       │         │
│  │        Text("Biomes") + Separator                            │         │
│  │        for i=0..BIOME_COUNT-1:                               │         │
│  │           draw swatch(IM_COL32(r,g,b,255))                   │         │
│  │           SameLine + Text(BIOMES[i].name)                    │         │
│  │     else:                                                    │         │
│  │        Text("Grayscale" or "Heatmap") + Separator            │         │
│  │        for i=0..15:                                          │         │
│  │           t = 1 - (i+0.5)/16                                 │         │
│  │           if mode==1: gray = t*255                           │         │
│  │           else:        heatmapColor(t, r,g,b)                │         │
│  │           draw filled rect(barW * segH/16)                   │         │
│  │        Dummy(barW,barH)  SameLine  Group( High / Low )       │         │
│  │  End()                                                       │         │
│  └──────────────────────────────────────────────────────────────┘         │
│                                                                           │
│  ┌── if view.showStats ∧ statsValid ─────────────────────────────┐        │
│  │  SetNextWindowPos(viewX+viewW-pad, pad, pivot=(1,0))          │        │
│  │  SetNextWindowBgAlpha(0.70)                                   │        │
│  │  if Begin("##Stats", overlayFlags|AutoResize):                │        │
│  │     Text("Terrain Stats") + Separator                         │        │
│  │     Text("Min:   %.3f", statMin)                              │        │
│  │     Text("Max:   %.3f", statMax)                              │        │
│  │     Text("Mean:  %.3f", statMean)                             │        │
│  │     Text("Land:  %.1f%%", statLandRatio*100)                  │        │
│  │     Text("Size:  %dx%d", params.terrainSize, ...)             │        │
│  │  End()                                                        │        │
│  └───────────────────────────────────────────────────────────────┘        │
│                                                                           │
└───────────────────────────────────────────────────────────────────────────┘
```

### 3.6 完整编码

```cpp
// ============================================================
// UIStyle.h
// 实现需求: FR-06
// ============================================================


#pragma once

#include "TerrainGenerator.h"

namespace UIStyle {

// 应用自定义UI主题参数
void ApplyCustomImGuiStyle();


// 控制面板内容
struct ControlPanelContext {
    TerrainParams&          params;                  // 地形参数
    ViewState&              view;                    // 视图参数

    char*  seedStr;          int seedStrSize;         // Terrain Seed
    char*  exportPath;       int exportPathSize;      // 高度图导出路径
    char*  exportColorPath;  int exportColorPathSize; // 彩图导出路径

    // UI → main 的信号
    bool&  needsRegen;

    // 导出请求
    bool exportHeightmapRequested = false;
    bool exportColormapRequested  = false;
};

// 绘制左侧控制面板
void DrawControlPanel(ControlPanelContext& ctx, float panelWidth, float framerate);


// 视口角落 overlay
void DrawViewportOverlays(const TerrainParams& params, const ViewState& view,
                          float viewX, float viewW, float viewH,
                          float statMin, float statMax,
                          float statMean, float statLandRatio,
                          bool statsValid);

}
```

```cpp
// ============================================================
// UIStyle.cpp
// 实现需求: FR-06
// ============================================================

#include "UIStyle.h"

#include "imgui/imgui.h"

#include <cstdio>
#include <cstdlib>
#include <random>
#include <algorithm>

namespace UIStyle {

// 导入JetBrainsMono字体
ImFont* LoadJetBrainsMono(ImGuiIO& io, float sizePixels) {
    const char* candidatePaths[] = {
        "../include/ttf/JetBrainsMono-Regular.ttf"
    };

    for (const char* path : candidatePaths) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(path, sizePixels);
        if (font) {
            return font;
        }
    }
    return nullptr;
}

// 控制面板各分区统一布局参数
struct PanelLayout {
    float panelWidth;
    float fontScale;
    float buttonWidth;
    float buttonHeight;
};

// FR-01: 核心噪声参数调节
void DrawNoiseSection(ControlPanelContext& ctx, bool& terrainParamChanged) {
    TerrainParams& params = ctx.params;
    ImGui::Text("Core Noise Parameters");
    if (ImGui::SliderFloat("Noise Scale", &params.noiseScale, 0.001f, 0.10f, "%.3f")) {
        ctx.needsRegen = true;
        terrainParamChanged = true;
    }
    if (ImGui::SliderInt("Octaves", &params.octaves, 1, 8)) {
        ctx.needsRegen = true;
        terrainParamChanged = true;
    }
    if (ImGui::SliderFloat("Persistence", &params.persistence, 0.3f, 0.8f, "%.2f")) {
        ctx.needsRegen = true;
        terrainParamChanged = true;
    }
}

// 绘制种子部分
void DrawSeedSection(ControlPanelContext& ctx, float panelWidth) {
    TerrainParams& params = ctx.params;
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Text("Terrain Seed (Unique ID)");
    float seedInputWidth = (panelWidth - style.WindowPadding.x * 2.0f) * 0.62f;
    ImGui::PushItemWidth(seedInputWidth);
    if (ImGui::InputText("##TerrainSeed", ctx.seedStr, static_cast<size_t>(ctx.seedStrSize),
                         ImGuiInputTextFlags_CharsHexadecimal |
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        // 用户按回车提交新 Seed：解析十六进制并解码所有参数
        uint64_t newSeed = std::strtoull(ctx.seedStr, nullptr, 16);
        decodeTerrainSeed(newSeed, params);
        uint64_t enc = encodeTerrainSeed(params);
        std::snprintf(ctx.seedStr, static_cast<size_t>(ctx.seedStrSize), "%016llX",
                      static_cast<unsigned long long>(enc));
        ctx.needsRegen = true;
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    float randomizeBtnWidth = (panelWidth - style.WindowPadding.x * 2.0f) - seedInputWidth - style.ItemInnerSpacing.x;
    randomizeBtnWidth = std::max(randomizeBtnWidth, 60.0f);
    if (ImGui::Button("Randomize", ImVec2(randomizeBtnWidth, 0.0f))) {
        // 使用独立 mt19937，不依赖全局 rand() 状态
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 0xFFFF);
        uint64_t rSeed = 0;
        for (int i = 0; i < 4; ++i) {
            rSeed = (rSeed << 16) ^ static_cast<uint64_t>(dist(rng));
        }
        decodeTerrainSeed(rSeed, params);
        uint64_t enc = encodeTerrainSeed(params);
        std::snprintf(ctx.seedStr, static_cast<size_t>(ctx.seedStrSize), "%016llX",
                      static_cast<unsigned long long>(enc));
        ctx.needsRegen = true;
    }
}

// 绘制地形参数调节
void DrawShapeSection(ControlPanelContext& ctx, bool& terrainParamChanged) {
    TerrainParams& params = ctx.params;
    ImGui::Text("Terrain Shape Parameters");
    if (ImGui::SliderFloat("Sea Level", &params.seaLevel, 0.0f, 1.0f, "%.2f")) {
        ctx.needsRegen = true; terrainParamChanged = true;
    }
    if (ImGui::SliderFloat("Height Contrast", &params.heightContrast, 0.1f, 3.0f, "%.2f")) {
        ctx.needsRegen = true; terrainParamChanged = true;
    }
    if (ImGui::SliderInt("Terrain Size", &params.terrainSize, 64, 512)) {
        ctx.needsRegen = true; terrainParamChanged = true;
    }
}

// 绘制显示模式
void DrawDisplaySection(ControlPanelContext& ctx, float panelWidth) {
    ViewState& view = ctx.view;
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGui::Text("Display Parameters");
    const char* modes[] = {"Biome Color", "Grayscale", "Heatmap"};
    if (ImGui::Combo("Display Mode", &view.displayMode, modes, 3)) ctx.needsRegen = true;
    ImGui::Checkbox("Show Legend", &view.showLegend);
    ImGui::Checkbox("Show Stats", &view.showStats);
    ImGui::SliderFloat("View Zoom", &view.viewZoom, 0.5f, 5.0f, "%.1f");
    ImGui::Checkbox("Enable Mouse Drag to Pan", &view.enablePan);
    float resetViewW = panelWidth - style.WindowPadding.x * 2.0f;
    if (ImGui::Button("Reset View", ImVec2(resetViewW, 0.0f))) {
        view.viewOffsetX = 0.0f;
        view.viewOffsetY = 0.0f;
    }
}

// 绘制交互控制
void DrawControlsSection(ControlPanelContext& ctx, const PanelLayout& layout,
                         bool& terrainParamChanged) {
    TerrainParams& params = ctx.params;
    ViewState& view = ctx.view;
    ImGui::Text("Controls");

    // FR-04-01: 自动生成开关
    ImGui::Checkbox("Auto Generate", &view.autoGenerate);

    // FR-04-02: 手动触发一次地形重生成
    if (ImGui::Button("Generate Terrain", ImVec2(layout.buttonWidth, layout.buttonHeight))) {
        ctx.needsRegen = true;
    }
    // FR-04-03: 一键恢复所有参数为默认值（地形 + 视图）
    if (ImGui::Button("Reset Parameters", ImVec2(layout.buttonWidth, layout.buttonHeight))) {
        TerrainGenerator::resetParams(params);
        TerrainGenerator::resetViewState(view);
        ctx.needsRegen = true;
        terrainParamChanged = true;
    }
}

// 绘制预设方案
void DrawPresetsSection(ControlPanelContext& ctx, const PanelLayout& layout,
                        bool& terrainParamChanged) {
    TerrainParams& params = ctx.params;
    ImGui::Text("Presets");
    float halfW = layout.buttonWidth / 2 - 5;
    if (ImGui::Button("Smooth Plains", ImVec2(halfW, layout.buttonHeight))) {
        TerrainGenerator::applyPresetSmoothPlains(params);
        ctx.needsRegen = true; terrainParamChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rugged Mountains", ImVec2(halfW, layout.buttonHeight))) {
        TerrainGenerator::applyPresetRuggedMountains(params);
        ctx.needsRegen = true; terrainParamChanged = true;
    }
    if (ImGui::Button("Large Continent", ImVec2(halfW, layout.buttonHeight))) {
        TerrainGenerator::applyPresetLargeContinent(params);
        ctx.needsRegen = true; terrainParamChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fragmented Isles", ImVec2(halfW, layout.buttonHeight))) {
        TerrainGenerator::applyPresetFragmentedIsles(params);
        ctx.needsRegen = true; terrainParamChanged = true;
    }
}

// 绘制数据导出
void DrawExportSection(ControlPanelContext& ctx, const PanelLayout& layout) {
    ImGui::Text("Export");
    ImGui::InputText("Heightmap File", ctx.exportPath, static_cast<size_t>(ctx.exportPathSize));
    if (ImGui::Button("Export Heightmap", ImVec2(layout.buttonWidth, layout.buttonHeight))) {
        ctx.exportHeightmapRequested = true;
    }
    ImGui::InputText("Colormap File", ctx.exportColorPath, static_cast<size_t>(ctx.exportColorPathSize));
    if (ImGui::Button("Export Colormap", ImVec2(layout.buttonWidth, layout.buttonHeight))) {
        ctx.exportColormapRequested = true;
    }
}




// 自定义UI参数
void ApplyCustomImGuiStyle() {
    ImGuiIO& io = ImGui::GetIO();

    // 加载JetBrainsMono字体
    io.Fonts->Clear();
    ImFont* font = LoadJetBrainsMono(io, 16.0f);
    io.FontDefault = font ? font : io.Fonts->AddFontDefault();

    ImGuiStyle& s = ImGui::GetStyle();

    // 圆角
    s.WindowRounding = 12.0f;    s.ChildRounding     = 10.0f;
    s.FrameRounding  = 8.0f;     s.PopupRounding     = 10.0f;
    s.ScrollbarRounding = 10.0f; s.GrabRounding      = 6.0f;
    s.TabRounding    = 8.0f;

    // 间距与边框
    s.WindowPadding    = ImVec2(14.0f, 14.0f);
    s.FramePadding     = ImVec2(8.0f, 5.0f);
    s.ItemSpacing      = ImVec2(8.0f, 8.0f);
    s.ItemInnerSpacing = ImVec2(8.0f, 4.0f);
    s.IndentSpacing    = 22.0f;    s.ScrollbarSize    = 12.0f;
    s.WindowBorderSize = 1.0f;     s.ChildBorderSize  = 1.0f;
    s.WindowMinSize    = ImVec2(320.0f, 200.0f);

    // 自定义配色表
    struct Col { ImGuiCol idx; ImVec4 col; };
    static const Col cols[] = {
        // 背景
        {ImGuiCol_WindowBg,             ImVec4(0.10f, 0.10f, 0.14f, 0.55f)},
        {ImGuiCol_ChildBg,              ImVec4(0.12f, 0.12f, 0.18f, 0.40f)},
        {ImGuiCol_PopupBg,              ImVec4(0.10f, 0.10f, 0.16f, 0.85f)},
        {ImGuiCol_MenuBarBg,            ImVec4(0.12f, 0.12f, 0.18f, 0.65f)},
        // 边框
        {ImGuiCol_Border,               ImVec4(0.65f, 0.75f, 0.95f, 0.35f)},
        {ImGuiCol_BorderShadow,         ImVec4(0.00f, 0.00f, 0.00f, 0.00f)},
        // 文字
        {ImGuiCol_Text,                 ImVec4(0.95f, 0.95f, 0.98f, 1.00f)},
        {ImGuiCol_TextDisabled,         ImVec4(0.55f, 0.55f, 0.60f, 1.00f)},
        // Frame
        {ImGuiCol_FrameBg,              ImVec4(0.16f, 0.16f, 0.22f, 0.55f)},
        {ImGuiCol_FrameBgHovered,       ImVec4(0.25f, 0.30f, 0.45f, 0.70f)},
        {ImGuiCol_FrameBgActive,        ImVec4(0.30f, 0.40f, 0.60f, 0.85f)},
        // 标题栏
        {ImGuiCol_TitleBg,              ImVec4(0.10f, 0.10f, 0.16f, 0.50f)},
        {ImGuiCol_TitleBgActive,        ImVec4(0.18f, 0.22f, 0.36f, 0.75f)},
        {ImGuiCol_TitleBgCollapsed,     ImVec4(0.08f, 0.08f, 0.12f, 0.50f)},
        // 滚动条
        {ImGuiCol_ScrollbarBg,          ImVec4(0.08f, 0.08f, 0.12f, 0.40f)},
        {ImGuiCol_ScrollbarGrab,        ImVec4(0.35f, 0.35f, 0.42f, 0.65f)},
        {ImGuiCol_ScrollbarGrabHovered, ImVec4(0.45f, 0.50f, 0.65f, 0.85f)},
        {ImGuiCol_ScrollbarGrabActive,  ImVec4(0.55f, 0.60f, 0.80f, 0.95f)},
        // 蓝色强调色
        {ImGuiCol_CheckMark,            ImVec4(0.45f, 0.65f, 1.00f, 1.00f)},
        {ImGuiCol_SliderGrab,           ImVec4(0.45f, 0.65f, 1.00f, 1.00f)},
        {ImGuiCol_SliderGrabActive,     ImVec4(0.60f, 0.75f, 1.00f, 1.00f)},
        // 按钮
        {ImGuiCol_Button,               ImVec4(0.22f, 0.28f, 0.40f, 0.70f)},
        {ImGuiCol_ButtonHovered,        ImVec4(0.35f, 0.50f, 0.80f, 0.85f)},
        {ImGuiCol_ButtonActive,         ImVec4(0.45f, 0.60f, 0.95f, 0.95f)},
        // Header
        {ImGuiCol_Header,               ImVec4(0.25f, 0.32f, 0.48f, 0.70f)},
        {ImGuiCol_HeaderHovered,        ImVec4(0.35f, 0.45f, 0.65f, 0.85f)},
        {ImGuiCol_HeaderActive,         ImVec4(0.45f, 0.55f, 0.75f, 0.95f)},
        // 分隔线
        {ImGuiCol_Separator,            ImVec4(0.50f, 0.55f, 0.70f, 0.35f)},
        {ImGuiCol_SeparatorHovered,     ImVec4(0.40f, 0.50f, 0.75f, 0.65f)},
        {ImGuiCol_SeparatorActive,      ImVec4(0.50f, 0.65f, 1.00f, 0.95f)},
        // 缩放手柄
        {ImGuiCol_ResizeGrip,           ImVec4(0.45f, 0.55f, 0.75f, 0.30f)},
        {ImGuiCol_ResizeGripHovered,    ImVec4(0.50f, 0.65f, 0.95f, 0.75f)},
        {ImGuiCol_ResizeGripActive,     ImVec4(0.60f, 0.75f, 1.00f, 0.95f)},
        // 标签
        {ImGuiCol_Tab,                  ImVec4(0.18f, 0.22f, 0.30f, 0.70f)},
        {ImGuiCol_TabHovered,           ImVec4(0.35f, 0.50f, 0.80f, 0.85f)},
        {ImGuiCol_TabActive,            ImVec4(0.30f, 0.40f, 0.62f, 0.95f)},
        {ImGuiCol_TabUnfocused,         ImVec4(0.14f, 0.16f, 0.22f, 0.70f)},
        {ImGuiCol_TabUnfocusedActive,   ImVec4(0.22f, 0.26f, 0.36f, 0.85f)},
    };
    for (const auto& e : cols) s.Colors[e.idx] = e.col;
}


// 绘制左侧控制面板
void DrawControlPanel(ControlPanelContext& ctx, float panelWidth, float framerate) {

    bool terrainParamChanged = false;

    // 窗口设置
    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("Control Panel", nullptr, windowFlags)) {
        const ImGuiStyle& style = ImGui::GetStyle();

        // FR-06-02: 字体和按钮高度随面板宽度动态缩放
        float fontScale = std::clamp(panelWidth / 480.0f, 1.0f, 1.6f);
        ImGui::SetWindowFontScale(fontScale);

        PanelLayout layout{panelWidth, fontScale,
                           panelWidth - 20.0f * fontScale,
                           28.0f * fontScale};

        ImGui::Text("Terrain Generator");
        ImGui::Separator();

        // FR-01: 核心噪声参数
        DrawNoiseSection(ctx, terrainParamChanged);

        // Terrain Seed 输入与随机化
        DrawSeedSection(ctx, panelWidth);
        ImGui::Separator();

        // FR-02: 地形形态参数
        DrawShapeSection(ctx, terrainParamChanged);
        ImGui::Separator();

        // FR-03: 显示参数
        DrawDisplaySection(ctx, panelWidth);
        ImGui::Separator();

        // FR-04: 交互控制
        DrawControlsSection(ctx, layout, terrainParamChanged);
        ImGui::Separator();

        // FR-04-04: 预设方案
        DrawPresetsSection(ctx, layout, terrainParamChanged);
        ImGui::Separator();

        // FR-05: 数据导出
        DrawExportSection(ctx, layout);

        // 在所有分区之后统一刷新 Seed 文本：
        // 修复原实现中 Shape/Reset/Presets 改动在 Seed 区之后执行、
        // 导致这些参数变化不更新 Seed 显示的 bug。
        if (terrainParamChanged) {
            uint64_t s = encodeTerrainSeed(ctx.params);
            std::snprintf(ctx.seedStr, static_cast<size_t>(ctx.seedStrSize), "%016llX",
                          static_cast<unsigned long long>(s));
        }

        ImGui::Separator();
        ImGui::Text("FPS: %.1f", framerate);
    }
    ImGui::End();
}


// 绘制统计与图例
void DrawViewportOverlays(const TerrainParams& params, const ViewState& view,
                          float viewX, float viewW, float viewH,
                          float statMin, float statMax,
                          float statMean, float statLandRatio,
                          bool statsValid) {

    const float pad = 10.0f;

    // 统计与图例窗口设置
    const ImGuiWindowFlags overlayFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus;


    // 图例
    if (view.showLegend) {

        ImGui::SetNextWindowPos(ImVec2(viewX + pad, viewH - pad),
                                ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.70f);

        if (ImGui::Begin("##Legend", nullptr,
                         overlayFlags | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SetWindowFontScale(1.2f);
            if (view.displayMode == 0) {

                // Biome：列出所有群系色块与名称
                ImGui::TextUnformatted("Biomes");
                ImGui::Separator();
                const float swatch = 20.0f;
                for (int i = 0; i < BIOME_COUNT; ++i) {
                    const BiomeInfo& b = BIOMES[i];
                    ImVec2 p0 = ImGui::GetCursorScreenPos();
                    ImVec2 p1(p0.x + swatch, p0.y + swatch);
                    ImU32 col = IM_COL32(b.r, b.g, b.b, 255);
                    ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, col);
                    ImGui::Dummy(ImVec2(swatch, swatch));
                    ImGui::SameLine();
                    ImGui::TextUnformatted(b.name);
                }
            } else {

                const int segs = 16;
                const float barW = 22.0f;
                const float barH = 180.0f;

                ImGui::TextUnformatted(view.displayMode == 1 ? "Grayscale" : "Heatmap");
                ImGui::Separator();

                ImVec2 barPos = ImGui::GetCursorScreenPos();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                for (int i = 0; i < segs; ++i) {
                    // 顶部 i=0 对应 height=1.0；底部 i=segs-1 对应 height=0.0
                    float t = 1.0f - (i + 0.5f) / segs;
                    unsigned char r, g, b;
                    if (view.displayMode == 1) {

                        // Grayscale: 白 -> 黑
                        unsigned char gray = static_cast<unsigned char>(t * 255);
                        r = g = b = gray;
                    } else {

                        // Heatmap: 蓝 -> 绿 -> 红
                        TerrainGenerator::heatmapColor(t, r, g, b);
                    }
                    float y0 = barPos.y + (barH * i) / segs;
                    float y1 = barPos.y + (barH * (i + 1)) / segs;
                    dl->AddRectFilled(ImVec2(barPos.x, y0),
                                      ImVec2(barPos.x + barW, y1),
                                      IM_COL32(r, g, b, 255));
                }

                // 占位让窗口尺寸包住渐变条
                ImGui::Dummy(ImVec2(barW, barH));
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::TextUnformatted("High");
                ImGui::Dummy(ImVec2(0.0f, barH - 2.0f * ImGui::GetTextLineHeightWithSpacing()));
                ImGui::TextUnformatted("Low");
                ImGui::EndGroup();
            }
        }
        ImGui::End();
    }


    // 统计
    if (view.showStats && statsValid) {
        ImGui::SetNextWindowPos(ImVec2(viewX + viewW - pad, pad),
                                ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.70f);

        if (ImGui::Begin("##Stats", nullptr,
                         overlayFlags | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("Terrain Stats");
            ImGui::Separator();
            ImGui::Text("Min:   %.3f", statMin);
            ImGui::Text("Max:   %.3f", statMax);
            ImGui::Text("Mean:  %.3f", statMean);
            ImGui::Text("Land:  %.1f%%", statLandRatio * 100.0f);
            ImGui::Text("Size:  %dx%d", params.terrainSize, params.terrainSize);
        }
        ImGui::End();
    }
}

}
```
