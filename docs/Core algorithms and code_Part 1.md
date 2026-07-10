# TerrainGenerator 核心算法与编码


## 一、PerlinNoise2D.{h,cpp}

### 1.1 排列表生成算法

构造一个长度 512 的整数数组 `p[]`，使得 `p[0..255]` 是 `0..255` 的随机排列，`p[256..511]` 是 `p[0..255]` 的重复。该表在后续 `noise()` 中被用于把 2D 单元格的角点映射到 4 个梯度方向。

```
┌──────────────── PerlinNoise2D(seed) ─────────────────┐
│                                                      │
│  ┌────── 初始化 ───────┐                             │
│  │ p.resize(512)      │                              │
│  │ rng = mt19937(seed)│                              │
│  └────────────────────┘                              │
│                                                      │
│  ┌── 构造 permutation[0..255] = [0,1,2,...,255] ──┐  │
│  │ for i = 0..255:                                │  │
│  │     permutation[i] = i                         │  │
│  └────────────────────────────────────────────────┘  │
│                                                      │
│  ┌──────────── Fisher-Yates 洗牌 ─────────────────┐  │
│  │ for i = 255 down to 1:                         │  │
│  │   ┌──────────┐                                 │  │
│  │   │ j = rand  │                                │  │
│  │   │  in[0,i] │                                 │  │
│  │   └────┬─────┘                                 │  │
│  │        ▼                                       │  │
│  │   ┌────────────────┐                           │  │
│  │   │ swap(perm[i],  │                           │  │
│  │   │      perm[j])  │                           │  │
│  │   └────────────────┘                           │  │
│  └────────────────────────────────────────────────┘  │
│                                                      │
│  ┌── 重复填充 p[0..511] ────────────────────────┐     │
│  │ for i = 0..511:                             │     │
│  │   ┌────────────────┐                        │     │
│  │   │ p[i] = perm[   │                        │     │
│  │   │   i mod 256]   │                        │     │
│  │   └────────────────┘                        │     │
│  └─────────────────────────────────────────────┘     │
│                                                      │
└──────────────────────────────────────────────────────┘
```


### 1.2 noise() 采样算法

```
┌──────────────── noise(x, y) ─────────────────────────┐
│                                                      │
│  ┌────── 单元定位 + 小数部分 ───────┐                 │
│  │ X = floor(x) & 255              │                 │
│  │ Y = floor(y) & 255              │                 │
│  │ xf = x - floor(x)               │                 │
│  │ yf = y - floor(y)               │                 │
│  └─────────────────────────────────┘                 │
│                                                      │
│  ┌────────────── 平滑曲线 ──────────────┐             │
│  │ u = fade(xf)                        │             │
│  │ v = fade(yf)                        │             │
│  └─────────────────────────────────────┘             │
│                                                      │
│  ┌────── 4 角哈希 ──────────────────────┐            │
│  │ AA = p[ p[X]   + Y   ]               │            │
│  │ BA = p[ p[X+1] + Y   ]               │            │
│  │ AB = p[ p[X]   + Y+1 ]               │            │
│  │ BB = p[ p[X+1] + Y+1 ]               │            │
│  └──────────────────────────────────────┘            │
│                                                      │
│  ┌── 双线性插值 (X 方向) ────────────────────┐        │
│  │ x1 = lerp(u, grad(AA, xf,   yf),         │        │
│  │                grad(BA, xf-1, yf))       │        │
│  │ x2 = lerp(u, grad(AB, xf,   yf-1),       │        │
│  │                grad(BB, xf-1, yf-1))     │        │
│  └──────────────────────────────────────────┘        │
│                                                      │
│  ┌── Y 方向插值 ───────────┐                          │
│  │ return lerp(v, x1, x2) │                          │
│  └────────────────────────┘                          │
│                                                      │
└──────────────────────────────────────────────────────┘
```

其中， fade()、grad()、lerp() 三个函数的NS图如下:

```
┌────────── fade(t) ──────────┐
│ t³·(t·(t·6 - 15) + 10)      │
│ (按 Horner 法则展开 6t⁵-     │
│  15t⁴+10t³)                 │
└─────────────────────────────┘
```

```
┌────────── grad(hash, x, y) ───────────┐
│ h = hash & 3  (取低 2 位)             │
├───────────────────────────────────────┤
│           h < 2 ?                     │
│     ┌───────────┬───────────┐         │
│  是 │ u = x     │ 否        │ u = y   │
│     │ v = y     │           │ v = x   │
│     └───────────┴───────────┘         │
├───────────────────────────────────────┤
│        (h & 1) == 0 ?                 │
│     ┌───────────┬───────────┐         │
│  是 │ u_s = u   │ 否        │ u_s = -u│
│     └───────────┴───────────┘         │
├───────────────────────────────────────┤
│        (h & 2) == 0 ?                 │
│     ┌───────────┬───────────┐         │
│  是 │ v_s = v   │ 否        │ v_s = -v│
│     └───────────┴───────────┘         │
├───────────────────────────────────────┤
│ 返回 u_s + v_s                        │
└───────────────────────────────────────┘

对应 4 个梯度方向:
h=0: (+x, +y)        h=1: (-x, +y)
      ↗                  ↖
       ·                  ·
      ↙                  ↘
h=3: (+x, -y)        h=2: (-x, -y)
```

```
┌───────── lerp(t, a, b) ────────┐
│  delta = b - a;                │
│  res = a + t * delta;          │
│  return res;                   │
└────────────────────────────────┘
```

### 1.3 八度叠加 octaveNoise() 算法

把多个不同频率/振幅的 `noise()` 叠加, 得到 1/f 频谱的类自然纹理, 然后归一化到 [-1, 1]。

```
┌───── octaveNoise(x, y, octaves, persistence) ─────────┐
│                                                       │
│  ┌──── 初始化 ────────────────────────┐                │
│  │ total = 0;   maxValue = 0;         │               │
│  │ freq = 1;    amp = 1;              │               │
│  └────────────────────────────────────┘               │
│                                                       │
│  ┌── for i = 0 ~ octaves-1 ────────────────────────┐  │
│  │                                                 │  │
│  │  ┌───────────────────────────────────────────┐  │  │
│  │  │ total    += noise(x·freq, y·freq) · amp   │  │  │
│  │  └───────────────────────────────────────────┘  │  │
│  │  ┌───────────────────────────────────────────┐  │  │
│  │  │ maxValue += amp                           │  │  │
│  │  └───────────────────────────────────────────┘  │  │
│  │  ┌───────────────────────────────────────────┐  │  │
│  │  │ amp  *= persistence                       │  │  │
│  │  └───────────────────────────────────────────┘  │  │
│  │  ┌───────────────────────────────────────────┐  │  │
│  │  │ freq *= 2                                 │  │  │
│  │  └───────────────────────────────────────────┘  │  │
│  │                                                 │  │
│  └─────────────────────────────────────────────────┘  │
│                                                       │
│  ┌────────────────────────────┐                       │
│  │ return total / maxValue    │                       │
│  └────────────────────────────┘                       │
│                                                       │
└───────────────────────────────────────────────────────┘
```

### 1.4 完整编码

```cpp
// ============================================================
// PerlinNoise.h
// PerlinNoise2D 实现
// 实现需求: FR-01（核心噪声参数调节）、DR-07（噪声算法内部数据）
// ============================================================
#pragma once

#include <vector>
#include <random>

class PerlinNoise2D {
private:
    // DR-07: 排列表
    std::vector<int> p;

    // 独立随机数
    std::mt19937 rng;

    double fade(double t) const;
    double lerp(double t, double a, double b) const;
    double grad(int hash, double x, double y) const;

public:
    // 用 seed 初始化排列表
    explicit PerlinNoise2D(unsigned int seed = 0);

    // 单层噪声, 并归一化输出 [-1, 1]
    double noise(double x, double y) const;

    // 多八度叠加, 归一化输出 [-1, 1] 
    // octaves: 叠加层数 (1~8)
    // persistence: 振幅衰减系数 (0.3~0.8)
    double octaveNoise(double x, double y,
                       int octaves, double persistence) const;
};
```

```cpp
// ============================================================
// PerlinNoise.cpp
// PerlinNoise2D 实现
// 实现需求: FR-01（核心噪声参数调节）、DR-07（噪声算法内部数据）
// ============================================================

#include "PerlinNoise.h"

#include <cmath>
#include <algorithm>



// DR-07: 256 随机排列 + 镜像重复到 512
PerlinNoise2D::PerlinNoise2D(unsigned int seed) : rng(seed) {
    p.resize(512);
    std::vector<int> permutation(256);
    for (int i = 0; i < 256; i++) permutation[i] = i;

    // Fisher-Yates洗牌，使用独立mt19937随机数
    for (int i = 255; i > 0; i--) {
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(rng);
        std::swap(permutation[i], permutation[j]);
    }

  
    for (int i = 0; i < 512; i++) {
        p[i] = permutation[i % 256];
    }
}



// fade: Perlin 原版 6t⁵ - 15t⁴ + 10t³
double PerlinNoise2D::fade(double t) const {
    return t * t * t * (t * (t * 6 - 15) + 10);
}



// lerp: 线性插值
double PerlinNoise2D::lerp(double t, double a, double b) const {
    return a + t * (b - a);
}



// grad: 哈希低 2 位选 4 个梯度方向之一
double PerlinNoise2D::grad(int hash, double x, double y) const {
    int h = hash & 3;
    double u = h < 2 ? x : y;
    double v = h < 2 ? y : x;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}



// noise: 单层 2D Perlin 噪声
double PerlinNoise2D::noise(double x, double y) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    x -= std::floor(x);     
    y -= std::floor(y);
    double u = fade(x);        
    double v = fade(y);

  
    int A  = p[X] + Y;
    int AA = p[A];
    int AB = p[A + 1];
    int B  = p[X + 1] + Y;
    int BA = p[B];
    int BB = p[B + 1];

    // lerp双线性插值
    return lerp(v,
                lerp(u, grad(p[AA], x,     y    ),
                        grad(p[BA], x - 1, y    )),
                lerp(u, grad(p[AB], x,     y - 1),
                        grad(p[BB], x - 1, y - 1)));
}



// octaveNoise: 多层叠加, 1/f 频谱, 归一化
double PerlinNoise2D::octaveNoise(double x, double y,
                                  int octaves, double persistence) const {
    double total = 0.0;
    double frequency = 1.0;
    double amplitude = 1.0;
    double maxValue = 0.0;

    for (int i = 0; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }

    return total / maxValue;
}
```




## 二、Terraingenerator.{h,cpp}

### 2.1 generate(params, displayMode) 地形生成算法

根据 `TerrainParams` 重建噪声对象, 遍历 `terrainSize × terrainSize` 网格, 写入 heightMap, 按 `displayMode` 三选一写入 colorMap。

```
┌────────────── generate(params, displayMode) ─────────────────┐
│                                                              │
│  ┌──── 基础设置 ─────────────────────────────────────┐        │
│  │ width  = params.terrainSize                       │       │
│  │ height = params.terrainSize                       │       │
│  │ noise  = make_unique<PerlinNoise2D>(              │       │
│  │             params.noiseSeed)                     │       │
│  └───────────────────────────────────────────────────┘       │
│                                                              │
│  ┌──── 分配数组 ──────────────────────────────────────┐       │
│  │ heightMap.resize(width * height)                  │       │
│  │ colorMap.resize(width * height * 3)                │      │
│  └────────────────────────────────────────────────────┘      │
│                                                              │
│  ┌── for y = 0 .. height - 1 ─────────────────────────┐      │
│  │                                                    │      │
│  │  ┌── for x = 0 .. width - 1 ─────────────────────┐ │      │
│  │  │                                               │ │      │
│  │  │  ┌──── 采样 + 后处理 ────┐                     │ │      │
│  │  │  │ nx = x * scale       │                     │ │      │
│  │  │  │ ny = y * scale       │                     │ │      │
│  │  │  │ h  = octaveNoise(    │                     │ │      │
│  │  │  │       nx, ny,        │                     │ │      │
│  │  │  │       octaves, pers) │                     │ │      │
│  │  │  │ h  = h * 0.5 + 0.5   │                     │ │      │
│  │  │  │ h  = applyContrast(  │                     │ │      │
│  │  │  │       h, contrast)   │                     │ │      │
│  │  │  │ heightMap[y*w+x] = h │                     │ │      │
│  │  │  └──────────────────────┘                     │ │      │
│  │  │                                               │ │      │
│  │  │  ┌──── idx = (y*w + x) * 3 ──┐                │ │      │
│  │  │  └───────────────────────────┘                │ │      │
│  │  │                                               │ │      │
│  │  │  ┌──── switch (displayMode) ────────────┐     │ │      │
│  │  │  │  ┌──0──┐ ┌──1──┐ ┌──2──┐             │     │ │      │
│  │  │  │  │case │ │case │ │case │             │     │ │      │
│  │  │  │  │ 0   │ │ 1   │ │ 2   │             │     │ │      │
│  │  │  │  │Bio. │ │Gray │ │Heat │             │     │ │      │
│  │  │  │  │     │ │     │ │     │             │     │ │      │
│  │  │  │  │geB. │ │h*255│ │heat.│             │     │ │      │
│  │  │  │  │BIOM.│ │灰度 │ │Col. │             │     │ │      │
│  │  │  │  └─────┘ └─────┘ └─────┘             │     │ │      │
│  │  │  │     │       │       │                │     │ │      │
│  │  │  │     └───┬───┴───┬───┘                │     │ │      │
│  │  │  │         ▼       ▼                    │     │ │      │
│  │  │  │  ┌──────────────────────┐            │     │ │      │
│  │  │  │  │colorMap[idx..idx+2]  │            │     │ │      │
│  │  │  │  │     = RGB            │            │     │ │      │
│  │  │  │  └──────────────────────┘            │     │ │      │
│  │  │  └──────────────────────────────────────┘     │ │      │
│  │  │                                               │ │      │
│  │  └───────────────────────────────────────────────┘ │      │
│  │                                                    │      │
│  └────────────────────────────────────────────────────┘      │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 2.2 getBiome(h, seaLevel) 群系判定

```
┌────────────────── getBiome(h, seaLevel) ───────────────────┐
│                                                            │
│  ┌── for i = 0 ~ BIOME_COUNT - 1 ───────────────────────┐  │
│  │                                                      │  │
│  │  ┌───────────────────────────────────────────────┐   │  │
│  │  │                                               │   │  │
│  │  │ b = BIOMES[i]; thresh = b.heightThreshold     │   │  │
│  │  └───────────────────────────────────────────────┘   │  │
│  │                                                      │  │
│  │  ┌──── switch (b.thresholdType) ─────────────────────┐  │
│  │  │  ┌─SEA_FRACTION─┐ ┌─SEA_OFFSET─┐ ┌──ABSOLUTE──┐   │  │
│  │  │  │ thresh =     │ │ thresh =   │ │  thresh    │   │  │
│  │  │  │   seaLevel * │ │   seaLevel │ │            │   │  │
│  │  │  │   thresh     │ │   + thresh │ │            │   │  │
│  │  │  └──────┬───────┘ └──────┬─────┘ └──────┬─────┘   │  │
│  │  │         └─────────┬──────┘              │         │  │
│  │  │                   ▼                     │         │  │
│  │  │              ┌──────────┐               │         │  │
│  │  │              │ thresh   │               │         │  │
│  │  │              └─────┬────┘               │         │  │
│  │  │                    │                    │         │  │
│  │  └────────────────────┼────────────────────┘         │  │
│  │                       │                              │  │
│  │            ┌──────────┴──────────┐                   │  │
│  │            │ h < thresh ?         │                  │  │
│  │            │   T ──► return i     │                  │  │
│  │            │   F ──► continue     │                  │  │
│  │            └──────────────────────┘                  │  │
│  │                                                      │  │
│  └──────────────────────────────────────────────────────┘  │
│                                                            │
│  ┌─────────────────────────────────────┐                   │
│  │ return BIOME_SNOW_PEAK              │                   │
│  └─────────────────────────────────────┘                   │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

### 2.3 Heatmap 配色插值算法

```
┌──────────── heatmapColor(t, &r, &g, &b) ─────────────────┐
│                                                          │
│  ┌────── if (t < 0.33) ──────────┐                       │
│  │       ┌────────────────────┐  │                       │
│  │       │ r = 0              │  │                       │
│  │       │ g = (t/0.33)*255   │  │                       │
│  │       │ b = 255            │  │                       │
│  │       └────────────────────┘  │                       │
│  └───────────────┬───────────────┘                       │
│                  │                                       │
│  ┌────── else if (t < 0.66) ─────┐                       │
│  │       ┌────────────────────┐  │                       │
│  │       │ r = ((t-0.33)      │  │                       │
│  │       │      /0.33) * 255  │  │                       │
│  │       │ g = 255            │  │                       │
│  │       │ b = (1-(t-0.33)    │  │                       │
│  │       │      /0.33) * 255  │  │                       │
│  │       └────────────────────┘  │                       │
│  └───────────────┬───────────────┘                       │
│                  │                                       │
│  ┌────── else ──────────────────┐                        │
│  │       ┌────────────────────┐  │                       │
│  │       │ r = 255            │  │                       │
│  │       │ g = (1-(t-0.66)    │  │                       │
│  │       │      /0.34) * 255  │  │                       │
│  │       │ b = 0              │  │                       │
│  │       └────────────────────┘  │                       │
│  └───────────────────────────────┘                       │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### 2.4 computeStats(heightMap, seaLevel) 高度图统计

```
┌────────── computeStats(heightMap, seaLevel) ─────────────┐
│                                                          │
│  ┌────── if (heightMap.empty()) ────┐                    │
│  │       ┌──────────────────────┐   │                    │
│  │       │ return {0,0,0,0,     │   │                    │
│  │       │         valid=false} │   │                    │
│  │       └──────────────────────┘   │                    │
│  └───────────────┬──────────────────┘                    │
│                  │                                       │
│  ┌────── 初始化 ─────────────────────┐                    │
│  │ mn = heightMap[0]                 │                   │
│  │ mx = heightMap[0]                 │                   │
│  │ sum = 0; land = 0                 │                   │
│  └──────────────┬────────────────────┘                   │
│                 ▼                                        │
│  ┌── for h in heightMap ─────────────────────────────┐   │
│  │                                                   │   │
│  │  ┌── if h < mn ──┐                                │   │
│  │  │   T  mn = h   │                                │   │
│  │  └────────────────┘                               │   │
│  │  ┌── if h > mx ──┐                                │   │
│  │  │   T  mx = h   │                                │   │
│  │  └────────────────┘                               │   │
│  │  ┌── sum += h ──────┐                             │   │
│  │  └──────────────────┘                             │   │
│  │  ┌── if h >= seaLevel ──┐                         │   │
│  │  │   T  land += 1       │                         │   │
│  │  └──────────────────────┘                         │   │
│  │                                                   │   │
│  └───────────────────────────────────────────────────┘   │
│                 ▼                                        │
│  ┌────── 计算最终结果 ─────────────────┐                  │
│  │ mean  = sum / heightMap.size()     │                  │
│  │ ratio = land / heightMap.size()    │                  │
│  │ return {mn, mx, mean, ratio,       │                  │
│  │         valid=true}                │                  │
│  └────────────────────────────────────┘                  │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### 2.5 64-bit 地形种子编码算法

位分配：
```
 ┌────────────────┬──────┬──────┬────────┬──────┬──────┬──────────┐
 │ bits[63:32]    │ 27:31│ 21:26│ 15:20  │ 10:14│ 6:0  │ 用途     │
 ├────────────────┼──────┼──────┼────────┼──────┼──────┼──────────┤
 │ noiseSeed      │  TS  │  HC  │  SL    │  PE  │  OC  │  NS      │
 │ 32 bits        │  5b  │  6b  │  6b    │  5b  │  3b  │  7b      │
 └────────────────┴──────┴──────┴────────┴──────┴──────┴──────────┘
```

```
┌────────── encodeTerrainSeed(params) ─────────────────────┐
│                                                           │
│  ┌── 量化 ──────────────────────────────────────────┐    │
│  │                                                    │    │
│  │  ┌─────────────────────────────────────────────┐  │    │
│  │  │ ns = clamp(round(noiseScale * 1000), 1,127) │  │    │
│  │  │ oc = clamp(octaves, 1, 8) - 1                │  │    │
│  │  │ pe = clamp(round((pers-0.30)/0.02), 0, 25)   │  │    │
│  │  │ sl = clamp(round(seaLevel*100), 0, 63)       │  │    │
│  │  │ hc = clamp(round((hc-0.10)/0.05), 0, 61)     │  │    │
│  │  │ ts = clamp((terrainSize-64)/16, 0, 28)       │  │    │
│  │  │ nseed = uint32_t(noiseSeed)                  │  │    │
│  │  └─────────────────────────────────────────────┘  │    │
│  │                                                    │    │
│  └────────────────────────────────────────────────────┘    │
│                                                           │
│  ┌── 拼接 ──────────────────────────────────────────┐    │
│  │ seed = 0                                          │    │
│  │ seed |= (ns    & 0x7F)                            │    │
│  │ seed |= (oc    & 0x07) << 7                       │    │
│  │ seed |= (pe    & 0x1F) << 10                      │    │
│  │ seed |= (sl    & 0x3F) << 15                      │    │
│  │ seed |= (hc    & 0x3F) << 21                      │    │
│  │ seed |= (ts    & 0x1F) << 27                      │    │
│  │ seed |= uint64_t(nseed) << 32                     │    │
│  └────────────────────────────────────────────────────┘    │
│                                                           │
│  ┌─────────────────────────────────────┐                 │
│  │ return seed                         │                 │
│  └─────────────────────────────────────┘                 │
│                                                           │
└───────────────────────────────────────────────────────────┘
```

### 2.6 64-bit 地形种子解码算法

```
┌────────── decodeTerrainSeed(seed, params) ────────────────┐
│                                                           │
│  ┌── 位提取 ───────────────────────────────────────┐      │
│  │ ns    = (seed)         & 0x7F                   │      │
│  │ oc    = (seed >> 7)    & 0x07                   │      │
│  │ pe    = (seed >> 10)   & 0x1F                   │      │
│  │ sl    = (seed >> 15)   & 0x3F                   │      │
│  │ hc    = (seed >> 21)   & 0x3F                   │      │
│  │ ts    = (seed >> 27)   & 0x1F                   │      │
│  │ nseed = uint32_t(seed >> 32)                    │      │
│  └─────────────────────────────────────────────────┘      │
│                                                           │
│  ┌── 反量化 + 写回 ────────────────────────────────┐       │
│  │ params.noiseScale     = ns / 1000.0f           │       │
│  │ params.octaves        = oc + 1                 │       │
│  │ params.persistence    = 0.30f + pe * 0.02f     │       │
│  │ params.seaLevel       = sl / 100.0f            │       │
│  │ params.heightContrast = 0.10f + hc * 0.05f     │       │
│  │ params.terrainSize    = 64 + ts * 16           │       │
│  │ params.noiseSeed      = int(nseed)             │       │
│  └────────────────────────────────────────────────┘       │
│                                                           │
└───────────────────────────────────────────────────────────┘
```


### 2.7 参数重置与四套预设参数算法

四套预设参数表:

| 预设 | noiseScale | octaves | persistence | contrast | seaLevel | 视觉特征 |
|------|-----------:|--------:|------------:|---------:|---------:|---------|
| Smooth Plains    | 0.020 | 2 | 0.30 | 0.60 | 0.45 | 平原广布, 海占 45% |
| Rugged Mountains | 0.080 | 7 | 0.70 | 1.50 | 0.25 | 多山, 细节极丰富 |
| Large Continent  | 0.015 | 5 | 0.50 | 1.00 | 0.35 | 大陆整块, 起伏适中 |
| Fragmented Isles | 0.060 | 6 | 0.60 | 1.20 | 0.50 | 群岛, 海占 50% |


NS 图（以 Smooth Plains 为例）
```
┌───── applyPresetSmoothPlains(params) ────────────────┐
│                                                      │
│  ┌────────────── 7 个字段赋值 ──────────────┐         │
│  │ params.noiseScale     = 0.02f           │         │
│  │ params.octaves        = 2               │         │
│  │ params.persistence    = 0.3f            │         │
│  │ params.heightContrast = 0.6f            │         │
│  │ params.seaLevel       = 0.45f           │         │
│  └─────────────────────────────────────────┘         │
│                                                      │
└──────────────────────────────────────────────────────┘

┌───── applyPresetRuggedMountains(params) ──────────────┐
│  params.noiseScale     = 0.08f                        │
│  params.octaves        = 7                            │
│  params.persistence    = 0.7f                         │
│  params.heightContrast = 1.5f                         │
│  params.seaLevel       = 0.25f                        │
└───────────────────────────────────────────────────────┘

┌───── applyPresetLargeContinent(params) ───────────────┐
│  params.noiseScale     = 0.015f                       │
│  params.octaves        = 5                            │
│  params.persistence    = 0.5f                         │
│  params.heightContrast = 1.0f                         │
│  params.seaLevel       = 0.35f                        │
└───────────────────────────────────────────────────────┘

┌───── applyPresetFragmentedIsles(params) ──────────────┐
│  params.noiseScale     = 0.06f                        │
│  params.octaves        = 6                            │
│  params.persistence    = 0.6f                         │
│  params.heightContrast = 1.2f                         │
│  params.seaLevel       = 0.50f                        │
└───────────────────────────────────────────────────────┘
```

### 2.8 完整编码

```cpp
// ============================================================
// TerrainGenerator.h
// TerrainGenerator 实现
// 实现需求: FR-01 ~ FR-04、DR-01 ~ DR-05
// ============================================================

#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "PerlinNoise.h"


// ========== DR-04: 参数 ==========
struct TerrainParams {
    // 核心噪声参数
    float noiseScale     = 0.05f;    // FR-01-01
    int   octaves        = 4;        // FR-01-02
    float persistence    = 0.5f;     // FR-01-03
    int   noiseSeed      = 42;       // FR-01-04

    // 地形参数
    float seaLevel       = 0.35f;    // FR-02-01
    float heightContrast = 1.0f;     // FR-02-02
    int   terrainSize    = 256;      // FR-02-03
};

// ========== 显示与视图状态参数 ==========
struct ViewState {
    int   displayMode   = 0;         // FR-03-01
    bool  showLegend    = false;     // FR-03-02
    bool  showStats     = false;     // FR-03-02
    float viewZoom      = 1.0f;      // FR-03-03
    float viewOffsetX   = 0.0f;      // FR-03-04
    float viewOffsetY   = 0.0f;      // FR-03-04
    bool  enablePan     = true;      // FR-03-05
    bool  autoGenerate  = true;      // FR-04-01
};

// ========== DR-08: Terrain Seed 编解码 ==========
uint64_t encodeTerrainSeed(const TerrainParams& params);
void     decodeTerrainSeed(uint64_t seed, TerrainParams& params);

// ========== DR-05: 群系 ==========
enum BiomeType {
    BIOME_DEEP_OCEAN = 0, 
    BIOME_OCEAN, 
    BIOME_SHALLOW_WATER,
    BIOME_BEACH, 
    BIOME_GRASSLAND, 
    BIOME_FOREST, 
    BIOME_HILLS,
    BIOME_MOUNTAIN, 
    BIOME_HIGH_MOUNTAIN, 
    BIOME_SNOW_PEAK,
    BIOME_COUNT
};

enum BiomeThresholdType {
    BIOME_THRESH_SEA_FRACTION,
    BIOME_THRESH_SEA_OFFSET,
    BIOME_THRESH_ABSOLUTE
};

struct BiomeInfo {
    const char* name;
    BiomeThresholdType thresholdType;
    float heightThreshold;
    unsigned char r, g, b;
};

extern const BiomeInfo BIOMES[BIOME_COUNT];

// ========== DR-09: 统计缓存 ==========
struct TerrainStats {
    float min = 0, max = 0, mean = 0, landRatio = 0;
    bool  valid = false;
};


class TerrainGenerator {
private:
    std::vector<float>         heightMap;    // DR-01
    std::vector<unsigned char> colorMap;     // DR-02
    int                        width = 0, height = 0;
    std::unique_ptr<PerlinNoise2D> noise;    // RAII

    static float      applyContrast(float h, float contrast);  // FR-02-02
    static BiomeType  getBiome(float h, float seaLevel);        // DR-05

public:
    void generate(const TerrainParams& params, int displayMode);

    const std::vector<float>&        getHeightMap() const;
    const std::vector<unsigned char>& getColorMap()  const;
    int getWidth()  const;
    int getHeight() const;

    static TerrainStats computeStats(const std::vector<float>& heightMap,
                                     float seaLevel);
    static void heatmapColor(float t,
                             unsigned char& r, unsigned char& g, unsigned char& b);

    static void resetParams(TerrainParams& params);
    static void resetViewState(ViewState& view);

    static void applyPresetSmoothPlains(TerrainParams& params);
    static void applyPresetRuggedMountains(TerrainParams& params);
    static void applyPresetLargeContinent(TerrainParams& params);
    static void applyPresetFragmentedIsles(TerrainParams& params);
};
```

```cpp
// ============================================================
// TerrainGenerator.cpp
// TerrainGenerator 实现
// 实现需求: FR-01 ~ FR-04、DR-01 ~ DR-05
// ============================================================

#include "TerrainGenerator.h"

#include <algorithm>



// DR-05: 群系配置表
const BiomeInfo BIOMES[BIOME_COUNT] = {
    {"Deep Ocean",      BIOME_THRESH_SEA_FRACTION, 0.5f,   0,   20,  80},
    {"Ocean",           BIOME_THRESH_SEA_FRACTION, 0.8f,   0,   60,  120},
    {"Shallow Water",   BIOME_THRESH_SEA_FRACTION, 1.0f,   0,   100, 180},
    {"Beach",           BIOME_THRESH_SEA_OFFSET,   0.05f,  210, 200, 140},
    {"Grassland",       BIOME_THRESH_ABSOLUTE,     0.50f,  50,  150, 50},
    {"Forest",          BIOME_THRESH_ABSOLUTE,     0.60f,  34,  100, 34},
    {"Hills",           BIOME_THRESH_ABSOLUTE,     0.70f,  120, 100, 60},
    {"Mountain",        BIOME_THRESH_ABSOLUTE,     0.80f,  100, 80,  60},
    {"High Mountain",   BIOME_THRESH_ABSOLUTE,     0.90f,  140, 140, 140},
    {"Snow Peak",       BIOME_THRESH_ABSOLUTE,     1.00f,  255, 255, 255}
};



// FR-02-02: 对比度
float TerrainGenerator::applyContrast(float h, float contrast) {
    h = (h - 0.5f) * contrast + 0.5f;
    return std::max(0.0f, std::min(1.0f, h));
}



// DR-05: 群系判定
BiomeType TerrainGenerator::getBiome(float h, float seaLevel) {
    for (int i = 0; i < BIOME_COUNT; ++i) {
        const BiomeInfo& b = BIOMES[i];
        float thresh = b.heightThreshold;
        switch (b.thresholdType) {
            case BIOME_THRESH_SEA_FRACTION: thresh = seaLevel * b.heightThreshold; break;
            case BIOME_THRESH_SEA_OFFSET:   thresh = seaLevel + b.heightThreshold; break;
            case BIOME_THRESH_ABSOLUTE:     break; // 已是 heightThreshold
        }
        if (h < thresh) return static_cast<BiomeType>(i);
    }
    return BIOME_SNOW_PEAK;
}



// FR-01~FR-03: generate() 地形生成
void TerrainGenerator::generate(const TerrainParams& params, int displayMode) {
    width  = params.terrainSize;
    height = params.terrainSize;

    // FR-01-04: noiseSeed 
    noise = std::make_unique<PerlinNoise2D>(params.noiseSeed);

    heightMap.resize(static_cast<size_t>(width) * height);
    colorMap.resize(static_cast<size_t>(width) * height * 3);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double nx = x * params.noiseScale;
            double ny = y * params.noiseScale;

            float h = static_cast<float>(noise->octaveNoise(
                nx, ny, params.octaves, params.persistence));
            h = h * 0.5f + 0.5f;
            h = applyContrast(h, params.heightContrast);

            heightMap[static_cast<size_t>(y) * width + x] = h;

            size_t idx = (static_cast<size_t>(y) * width + x) * 3;

            // FR-03-01: ColorMap
            if (displayMode == 0) {
                BiomeType biome = getBiome(h, params.seaLevel);
                colorMap[idx]     = BIOMES[biome].r;
                colorMap[idx + 1] = BIOMES[biome].g;
                colorMap[idx + 2] = BIOMES[biome].b;
            } else if (displayMode == 1) {
                unsigned char gray = static_cast<unsigned char>(h * 255);
                colorMap[idx]     = gray;
                colorMap[idx + 1] = gray;
                colorMap[idx + 2] = gray;
            } else {
                unsigned char r, g, b;
                heatmapColor(h, r, g, b);
                colorMap[idx]     = r;
                colorMap[idx + 1] = g;
                colorMap[idx + 2] = b;
            }
        }
    }
}


const std::vector<float>& TerrainGenerator::getHeightMap() const { return heightMap; }
const std::vector<unsigned char>& TerrainGenerator::getColorMap() const { return colorMap; }
int TerrainGenerator::getWidth()  const { return width; }
int TerrainGenerator::getHeight() const { return height; }



// FR-03-02: computeStats
TerrainStats TerrainGenerator::computeStats(
    const std::vector<float>& heightMap, float seaLevel) {
    TerrainStats s;
    if (heightMap.empty()) return s;

    float mn = heightMap[0], mx = heightMap[0], sum = 0.0f;
    long long landCount = 0;
    for (float h : heightMap) {
        if (h < mn) mn = h;
        if (h > mx) mx = h;
        sum += h;
        if (h >= seaLevel) ++landCount;
    }
    s.min        = mn;
    s.max        = mx;
    s.mean       = sum / static_cast<float>(heightMap.size());
    s.landRatio  = static_cast<float>(landCount)
                 / static_cast<float>(heightMap.size());
    s.valid      = true;
    return s;
}



// FR-03-01: Heatmap 配色
void TerrainGenerator::heatmapColor(float t,
                                    unsigned char& r,
                                    unsigned char& g,
                                    unsigned char& b) {
    if (t < 0.33f) {
        r = 0;
        g = static_cast<unsigned char>(t / 0.33f * 255);
        b = 255;
    } else if (t < 0.66f) {
        r = static_cast<unsigned char>((t - 0.33f) / 0.33f * 255);
        g = 255;
        b = static_cast<unsigned char>((1.0f - (t - 0.33f) / 0.33f) * 255);
    } else {
        r = 255;
        g = static_cast<unsigned char>((1.0f - (t - 0.66f) / 0.34f) * 255);
        b = 0;
    }
}



// FR-04-03: 参数重置
void TerrainGenerator::resetParams(TerrainParams& params) {
    params.noiseScale     = 0.05f;
    params.octaves        = 4;
    params.persistence    = 0.5f;
    params.noiseSeed      = 42;
    params.seaLevel       = 0.35f;
    params.heightContrast = 1.0f;
    params.terrainSize    = 256;
}

void TerrainGenerator::resetViewState(ViewState& view) {
    view.displayMode  = 0;
    view.showLegend   = false;
    view.showStats    = false;
    view.viewZoom     = 1.0f;
    view.viewOffsetX  = 0.0f;
    view.viewOffsetY  = 0.0f;
    view.enablePan    = true;
    view.autoGenerate = true;
}



// FR-04-04: 四套预设
void TerrainGenerator::applyPresetSmoothPlains(TerrainParams& params) {
    params.noiseScale     = 0.02f;
    params.octaves        = 2;
    params.persistence    = 0.3f;
    params.heightContrast = 0.6f;
    params.seaLevel       = 0.45f;
}

void TerrainGenerator::applyPresetRuggedMountains(TerrainParams& params) {
    params.noiseScale     = 0.08f;
    params.octaves        = 7;
    params.persistence    = 0.7f;
    params.heightContrast = 1.5f;
    params.seaLevel       = 0.25f;
}

void TerrainGenerator::applyPresetLargeContinent(TerrainParams& params) {
    params.noiseScale     = 0.015f;
    params.octaves        = 5;
    params.persistence    = 0.5f;
    params.heightContrast = 1.0f;
    params.seaLevel       = 0.35f;
}

void TerrainGenerator::applyPresetFragmentedIsles(TerrainParams& params) {
    params.noiseScale     = 0.06f;
    params.octaves        = 6;
    params.persistence    = 0.6f;
    params.heightContrast = 1.2f;
    params.seaLevel       = 0.50f;
}



// DR-08: Terrain Seed 编解码实现
uint64_t encodeTerrainSeed(const TerrainParams& params) {
    int ns = static_cast<int>(params.noiseScale * 1000.0f + 0.5f);
    if (ns < 1) ns = 1; else if (ns > 127) ns = 127;

    int oc = params.octaves;
    if (oc < 1) oc = 1; else if (oc > 8) oc = 8;
    oc -= 1;

    int pe = static_cast<int>((params.persistence - 0.30f) / 0.02f + 0.5f);
    if (pe < 0) pe = 0; else if (pe > 25) pe = 25;

    int sl = static_cast<int>(params.seaLevel * 100.0f + 0.5f);
    if (sl < 0) sl = 0; else if (sl > 63) sl = 63;

    int hc = static_cast<int>((params.heightContrast - 0.10f) / 0.05f + 0.5f);
    if (hc < 0) hc = 0; else if (hc > 61) hc = 61;

    int ts = (params.terrainSize - 64) / 16;
    if (ts < 0) ts = 0; else if (ts > 28) ts = 28;

    uint32_t nseed = static_cast<uint32_t>(params.noiseSeed);

    uint64_t seed = 0;
    seed |= static_cast<uint64_t>(ns   & 0x7F);
    seed |= static_cast<uint64_t>(oc   & 0x07) << 7;
    seed |= static_cast<uint64_t>(pe   & 0x1F) << 10;
    seed |= static_cast<uint64_t>(sl   & 0x3F) << 15;
    seed |= static_cast<uint64_t>(hc   & 0x3F) << 21;
    seed |= static_cast<uint64_t>(ts   & 0x1F) << 27;
    seed |= static_cast<uint64_t>(nseed) << 32;
    return seed;
}

void decodeTerrainSeed(uint64_t seed, TerrainParams& params) {
    int ns  = static_cast<int>(seed & 0x7F);
    int oc  = static_cast<int>((seed >> 7)  & 0x07);
    int pe  = static_cast<int>((seed >> 10) & 0x1F);
    int sl  = static_cast<int>((seed >> 15) & 0x3F);
    int hc  = static_cast<int>((seed >> 21) & 0x3F);
    int ts  = static_cast<int>((seed >> 27) & 0x1F);
    uint32_t nseed = static_cast<uint32_t>(seed >> 32);

    params.noiseScale     = ns / 1000.0f;
    params.octaves        = oc + 1;
    params.persistence    = 0.30f + pe * 0.02f;
    params.seaLevel       = sl / 100.0f;
    params.heightContrast = 0.10f + hc * 0.05f;
    params.terrainSize    = 64 + ts * 16;
    params.noiseSeed      = static_cast<int>(nseed);
}
```