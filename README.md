# TerrainGenerator

> **声明**：本项目为大学课程大作业，旨在综合运用 C++20、计算机图形学与程序化生成技术，实现一个基于柏林噪声的实时二维地形生成器。代码与文档仅供学习交流使用。

一个基于 C++20 构建的实时程序化二维地形生成器，采用柏林噪声（Perlin Noise）算法，通过自定义主题的 ImGui 界面在 OpenGL 3.0 上渲染。

---

## 项目简介

TerrainGenerator 是一款桌面应用程序，能够实时生成自然风格的地形高度图与彩色图。其核心实现了二维柏林噪声采样器，支持多八度分形布朗运动（fBm）叠加，并将高度场映射为生物群系、灰度或热力图三种可视化模式。

项目采用严格的分层架构（编排层 → 业务层 → 工具层 → 表现层 → 算法层），依赖方向单向，各模块可独立测试与替换。

### 核心特性

- **实时噪声生成** — 二维柏林噪声，支持调节缩放比例、八度数量、持久度与随机种子
- **三种显示模式** — 生物群系彩色模式（10 种群系的 Whittaker 风格分类）、灰度高度模式、热力图模式
- **交互式视口** — 鼠标滚轮缩放、左键拖拽平移、等比渲染防止拉伸变形
- **参数预设方案** — 平滑平原、崎岖山地、大型大陆、细碎群岛
- **Terrain Seed 系统** — 64 位编码的地形唯一标识，支持精准复现
- **数据导出** — 原生保存对话框，支持导出为 PGM（灰度）和 PPM（彩色）二进制格式
- **自定义 UI 主题** — 伪毛玻璃风格，字体与控件随面板宽度动态缩放

---

## 第三方依赖

本项目依赖三个第三方库，均已**内置**于仓库中，无需额外安装包管理器。

| 库                                                    | 版本  | 用途                                             | 位置                                  |
| ----------------------------------------------------- | ----- | ------------------------------------------------ | ------------------------------------- |
| **[Dear ImGui](https://github.com/ocornut/imgui)** | 1.89+ | 即时模式 GUI，用于控制面板与视口叠加层           | `include/imgui/`                    |
| **[GLEW](http://glew.sourceforge.net/)**（GL）     | 2.1+  | OpenGL 扩展加载（通过 `glew32s.lib` 静态链接） | `include/GL/`、`lib/glew32s.lib`  |
| **[GLFW](https://www.glfw.org/)**                  | 3.3+  | 窗口管理、输入处理与 OpenGL 上下文创建           | `include/GLFW/`、`lib/libglfw3.a` |

> `lib/` 目录还包含 `glfw3.dll` 和 `libglfw3dll.a`，用于动态链接场景。默认构建使用静态 GLEW（`GLEW_STATIC`）与静态 GLFW，生成自包含的可执行文件。

---

## 使用说明

### 启动应用

构建完成后，运行 `TerrainGenerator.exe`，将打开一个 1400×900 的窗口，左侧为固定的控制面板，右侧为地形预览视口。

### 基本操作流程

1. **调节噪声参数** — 在左侧面板拖动滑块修改 Noise Scale、Octaves、Persistence。开启 Auto Generate 后地形实时更新。
2. **使用预设方案** — 点击四个预设按钮快速切换地形风格：

   - **Smooth Plains**（平滑平原）— 平缓起伏的丘陵
   - **Rugged Mountains**（崎岖山地）— 密集陡峭的山峰
   - **Large Continent**（大型大陆）— 广阔连续的陆地
   - **Fragmented Isles**（细碎群岛）— 散布的岛屿群
3. **切换显示模式** — 通过下拉框在 Biome Color（生物群系）、Grayscale（灰度）、Heatmap（热力图）之间切换。
4. **视口导航** — 勾选"Enable Mouse Drag to Pan"后，滚轮缩放、左键拖拽平移。点击"Reset View"重置视图。
5. **导出地形数据** — 输入文件路径后点击"Export Heightmap"（导出 PGM）或"Export Colormap"（导出 PPM），将弹出原生保存对话框选择目标路径。

### Terrain Seed（地形种子）

"Terrain Seed"文本框中的 16 位十六进制字符串将所有影响地形形状的参数编码为 64 位唯一标识。输入种子并按回车可精准复现对应地形，点击"Randomize"则生成新的随机种子。

---

## 项目结构

```
TerrainGenerator/
├── include/                  # 公共头文件与第三方库
│   ├── PerlinNoise.h         # 算法层 — 柏林噪声接口
│   ├── TerrainGenerator.h    # 业务层 — 地形生成与参数定义
│   ├── Exporter.h            # 工具层 — PGM/PPM 导出
│   ├── UIStyle.h             # 表现层 — ImGui 主题与控制面板
│   ├── GL/                   # GLEW 头文件（第三方）
│   ├── GLFW/                 # GLFW 头文件（第三方）
│   ├── imgui/                # Dear ImGui 源码与头文件（第三方）
│   └── ttf/                  # JetBrainsMono 字体
├── src/                      # 模块实现
│   ├── main.cpp              # 编排层 — 主循环与渲染
│   ├── PerlinNoise.cpp       # 算法层
│   ├── TerrainGenerator.cpp  # 业务层
│   ├── Exporter.cpp          # 工具层
│   └── UIStyle.cpp           # 表现层
├── lib/                      # 预编译第三方库
├── docs/                     # 项目文档
│   ├── TerrainGenerator_Requirements.md
│   ├── TerrainGenerator_Architecture.md
│   ├── Core algorithms and code_Part 1.md
│   └── Core algorithms and code_Part 2.md
├── CMakeLists.txt            # CMake 构建脚本
└── README.md
```

---

## 项目文档

详细文档位于 [`docs/`](docs/) 目录：

- **[需求文档](docs/TerrainGenerator_Requirements.md)** — 功能需求与数据需求规格说明
- **[架构设计](docs/TerrainGenerator_Architecture.md)** — 分层设计、模块依赖与数据流
- **[核心算法与编码（一）](docs/Core%20algorithms%20and%20code_Part%201.md)** — PerlinNoise2D 模块算法图与代码详解
- **[核心算法与编码（二）](docs/Core%20algorithms%20and%20code_Part%202.md)** — main / Exporter / UIStyle 模块算法图与代码详解

---

## 致谢

- **Ken Perlin** — 柏林噪声发明者
- **Omar Cornut** — Dear ImGui 创建者
- GLFW 与 GLEW 开源社区
