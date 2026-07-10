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
