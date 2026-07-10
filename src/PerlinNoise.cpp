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