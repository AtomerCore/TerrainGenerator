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
