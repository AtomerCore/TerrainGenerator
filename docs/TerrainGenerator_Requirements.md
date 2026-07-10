# 基于 ImGui 框架与柏林噪声的程序化随机地形生成器（TerrainGenerator）

---


## 一、功能需求（Functional Requirements）

### FR-01：核心噪声参数调节（Core Noise Parameters）

系统提供四个 Perlin 噪声核心参数，修改后需要重新计算高度图。

- **FR-01-01 噪声缩放比例（Noise Scale）**：浮点滑块（范围 0.001~0.10），控制噪声空间缩放。数值越小地形越平缓，越大越细碎。
- **FR-01-02 八度数量（Octaves）**：整数滑块（范围 1~8），控制噪声叠加层数。层数越多细节越丰富，计算量也越大。
- **FR-01-03 持久度（Persistence）**：浮点滑块（范围 0.30~0.80），控制每层振幅衰减速度，决定地形粗糙程度。
- **FR-01-04 随机种子（Terrain Seed）**：以 16 位十六进制文本框 + "Randomize" 按钮提供。相同 Seed 生成完全一致的地形，支持精准复现与随机探索。Seed 由内部编解码机制维护（详见 DR-08）。

### FR-02：地形形态参数调节（Terrain Shape Parameters）

此类参数不改变噪声输入，仅调整高度值映射规则与采样网格。开启 Auto Generate 时，修改即触发重生成并实时生效。

- **FR-02-01 海平面高度（Sea Level）**：浮点滑块（范围 0.0~1.0），调整水域与陆地分界。拉高增大海洋面积，仅影响 Biome 模式下的颜色映射，不改变 heightMap 数值。
- **FR-02-02 高度对比度（Height Contrast）**：浮点滑块（范围 0.1~3.0），通过拉伸或压缩高度分布改变极端地形占比。
- **FR-02-03 地形尺寸（Terrain Size）**：整数滑块（范围 64~512），控制正方形网格分辨率。修改后需重新生成高度图与颜色图。

### FR-03：显示渲染参数切换（Display Rendering Parameters）

此类参数不影响地形数据，仅改变画面呈现方式，全部实时生效。

- **FR-03-01 显示模式切换（Display Mode）**：下拉框，支持生物群系彩色模式（Biome Color，默认）、灰度高度模式（Grayscale）、热力图模式（Heatmap）三种。
- **FR-03-02 辅助元素开关（Auxiliary Toggles）**：两个复选框，分别控制显示图例（Show Legend）、显示统计信息（Show Stats）。统计信息（min/max/mean/landRatio）由内部统计函数一次性计算后缓存，仅在重生成时刷新（详见 DR-09）。
- **FR-03-03 视图缩放（View Zoom）**：浮点滑块，范围 0.5~5.0，默认 1.0。仅调整预览比例，不改变生成结果。
- **FR-03-04 视图偏移（View Offset）**：表示地形在视口中的水平和垂直平移量。
- **FR-03-05 启用鼠标拖动平移（Enable Mouse Drag to Pan）**：复选框。开启后，在右侧视口内按住左键拖动可平移地形，滚轮可缩放 View Zoom。"Reset View" 按钮将偏移归零。

### FR-04：交互控制功能（Interaction Controls）

- **FR-04-01 自动生成开关（Auto Generate）**：复选框。开启后滑块实时重绘地形。
- **FR-04-02 重新生成按钮（Generate Button）**：手动触发一次地形重生成。
- **FR-04-03 重置参数按钮（Reset Button）**：一键恢复所有参数（地形参数 + 视图状态）为默认值。
- **FR-04-04 预设方案按钮（Preset Buttons）**：四套经典地形预设——Smooth Plains（平滑平原）、Rugged Mountains（崎岖山地）、Large Continent（大型大陆）、Fragmented Isles（细碎群岛）。

### FR-05：数据导出功能（Data Export）

- **FR-05-01 导出高度图（Export Heightmap）**：文本框 + 按钮。点击后弹出原生"另存为"对话框选择路径，导出为 PGM（P5）二进制灰度图。
- **FR-05-02 导出彩色图（Export Colormap）**：文本框 + 按钮。同上弹出原生对话框，导出为 PPM（P6）二进制彩图。

### FR-06：UI 界面与交互体验（UI/UX）

- **FR-06-01 固定左侧面板（Fixed Left Panel）**：不可拖动，宽度随窗口缩放自适应（占窗口宽度 32%，夹紧 480~640 像素）。
- **FR-06-02 字体与控件缩放（Dynamic Scaling）**：字体和按钮高度随面板宽度动态缩放，使控件在小窗口与大窗口下都保持合适的可读性与可点击性。
- **FR-06-03 自定义主题样式（Custom Theme）**：伪毛玻璃风格——半透明背景（让 OpenGL 渲染底色透过来）、大圆角边框、浅色高光边框、蓝色强调色，加载 JetBrainsMono 等宽字体。

---

## 二、数据需求（Data Requirements）

### DR-01：高度图数据（Height Map）

一维 float 数组，长度为 `terrainSize × terrainSize`（正方形网格），是地形生成的核心输出。所有形态参数和显示模式均基于该数据变换。

### DR-02：颜色图数据（Color Map）

一维 unsigned char 数组，长度为 `terrainSize × terrainSize × 3`（RGB）。在生成时根据显示模式计算：生物群系模式查表映射，灰度模式线性映射，热力图模式蓝绿红渐变映射。显示模式切换会触发重生成。

### DR-03：OpenGL 纹理对象（OpenGL Texture）

GPU 端 2D 纹理对象（`GL_TEXTURE_2D`），由系统初始化时创建，运行时通过原地刷新避免重复创建。承载 colorMap 的 GPU 上传载体。

### DR-04：参数配置数据（Parameter Configs）

参数遵循"单一职责"原则拆分为两个结构体：

- **地形形状参数**：核心噪声参数（噪声缩放、八度、持久度、随机种子）与地形形态参数（海平面、高度对比度、地形尺寸），参与 Terrain Seed 编解码。
- **视图/交互状态**：显示参数（显示模式、图例开关、统计开关、缩放、偏移、启用平移）与交互开关（自动生成），不参与 Seed 编解码。

两组参数在 UI 控件上双向绑定，每个字段都有合理的默认值与取值范围。

### DR-05：群系配置表（Biomes Table）

静态常量数组，包含 10 种群系：Deep Ocean、Ocean、Shallow Water、Beach、Grassland、Forest、Hills、Mountain、High Mountain、Snow Peak。

每种群系包含：

- **名称（name）**：显示用字符串。
- **阈值类型（thresholdType）**：决定"高度阈值"如何被解释。
- **高度阈值（heightThreshold）**：结合阈值类型使用的阈值数值。
- **RGB 颜色值（Red/Green/Blue）**：用于 Biome 模式上色。

阈值类型支持三种语义：相对于海平面的比例、相对于海平面的固定偏移、以及与海平面无关的绝对阈值。这一设计使群系选择完全由数据表驱动，新增/调整群系无需修改代码。

### DR-06：导出文件数据格式（Export File Format）

- **PGM（P5）**：连续像素字节的二进制灰度图，遵循 Netpbm 标准。
- **PPM（P6）**：连续 RGB 字节的二进制彩图，遵循 Netpbm 标准，兼容主流图像软件。

文件头包含格式标识、宽高与最大灰度值（255），随后为原始像素数据。

### DR-07：噪声算法内部数据（Noise Internal Data）

排列表（Permutation Table）：一维 int 数组，由随机排列索引重复构成。根据 seed 初始化，噪声采样时通过坐标哈希查表获取梯度方向。该表独立于全局随机状态，避免污染其他模块的随机行为。

### DR-08：地形唯一标识（Terrain Seed）

将"决定地形形状的参数"打包为一个 64 位整数，作为地形唯一 ID。不包含显示模式、图例、视图等纯显示状态。

位分配（共 64 位）：

| 字段           | 位宽    | 精度 / 范围                  |
| -------------- | ------- | ---------------------------- |
| noiseScale     | 7 bits  | 0.001 精度，范围 0.001~0.127 |
| octaves        | 3 bits  | 1~8                          |
| persistence    | 5 bits  | 0.02 精度，范围 0.30~0.80    |
| seaLevel       | 6 bits  | 0.01 精度，范围 0.00~0.63    |
| heightContrast | 6 bits  | 0.05 精度，范围 0.10~3.15    |
| terrainSize    | 5 bits  | 64~512，步长 16              |
| noiseSeed      | 32 bits | Perlin 排列表种子            |

UI 以 16 位十六进制字符串显示。用户在文本框中按回车提交新 Seed，会反解出全部参数并触发重生成；"Randomize" 按钮生成 64 位随机 Seed。

### DR-09：高度图统计缓存（TerrainStats）

Show Stats 显示的统计信息（min / max / mean / landRatio）由内部统计函数一次性遍历 heightMap 计算，结果缓存为 `TerrainStats` 结构。仅在重生成时刷新一次，避免每帧遍历 heightMap 造成性能浪费。heightMap 为空时 `valid=false`，UI 侧据此决定是否显示统计窗口。
