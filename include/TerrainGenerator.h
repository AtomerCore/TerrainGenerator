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
