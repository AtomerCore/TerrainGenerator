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
