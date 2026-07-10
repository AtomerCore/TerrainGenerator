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
