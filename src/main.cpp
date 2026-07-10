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
