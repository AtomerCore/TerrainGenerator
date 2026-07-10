// ============================================================
// Exporter.h
// 实现需求: FR-05（数据导出）、DR-06（导出文件数据格式）
// ============================================================


#pragma once

#include <string>
#include <vector>

namespace Exporter {

// 导出PGM（P5）二进制灰度图
bool exportHeightmap(const char* filename,
                     const std::vector<float>& heightMap,
                     int width, int height);

// 导出彩色图PPM（P6）二进制彩图
bool exportColormap(const char* filename,
                    const std::vector<unsigned char>& colorMap,
                    int width, int height);


// Windows原生“另存为”对话框
std::string pickSavePath(bool isColormap, const char* currentPath);

}
