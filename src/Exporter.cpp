// ============================================================
// Exporter.cpp
// 实现需求: FR-05（数据导出）、DR-06（导出文件数据格式）
// ============================================================


#include "Exporter.h"

#include <fstream>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>


namespace Exporter {

// 导出Heightmap
bool exportHeightmap(const char* filename,
                     const std::vector<float>& heightMap,
                     int width, int height) {
    if (heightMap.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // PGM(P5)文件头
    file << "P5\n";
    file << width << " " << height << "\n";
    file << "255\n";

    // 批量转换并一次写入
    std::vector<unsigned char> buffer;
    buffer.reserve(heightMap.size());
    for (float h : heightMap) {
        buffer.push_back(static_cast<unsigned char>(h * 255));
    }
    file.write(reinterpret_cast<const char*>(buffer.data()),
               static_cast<std::streamsize>(buffer.size()));

    return true;
}

// 导出Colormap
bool exportColormap(const char* filename,
                    const std::vector<unsigned char>& colorMap,
                    int width, int height) {
    if (colorMap.empty()) return false;

    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;

    // DR-06: PPM (P6) 文件头
    file << "P6\n";
    file << width << " " << height << "\n";
    file << "255\n";

    file.write(reinterpret_cast<const char*>(colorMap.data()), static_cast<std::streamsize>(colorMap.size()));
    return true;
}


// 弹出Windows原生“另存为”对话框
std::string pickSavePath(bool isColormap, const char* currentPath) {

    // 过滤器字符串：双 \0 结尾；数组形式保留嵌入的 \0
    static const wchar_t filterPgm[] = L"PGM Image (*.pgm)\0*.pgm\0All Files (*.*)\0*.*\0";
    static const wchar_t filterPpm[] = L"PPM Image (*.ppm)\0*.ppm\0All Files (*.*)\0*.*\0";

    // 把当前路径（UTF-8）转 UTF-16，取出文件名部分作为对话框初始文件名
    wchar_t wszFile[MAX_PATH] = {0};
    if (currentPath && *currentPath) {
        int wlen = MultiByteToWideChar(CP_UTF8, 0, currentPath, -1, nullptr, 0);
        std::wstring wCurrent(static_cast<size_t>(wlen), 0);
        MultiByteToWideChar(CP_UTF8, 0, currentPath, -1, wCurrent.data(), wlen);

        const wchar_t* base = wCurrent.c_str();
        const wchar_t* slash  = wcsrchr(wCurrent.c_str(), L'\\');
        const wchar_t* slash2 = wcsrchr(wCurrent.c_str(), L'/');
        // 修复原实现的 nullptr 比较 UB：仅当 slash2 非空且（slash 为空或 slash2 更靠后）时采用 slash2
        if (slash2 && (!slash || slash2 > slash)) slash = slash2;
        if (slash) base = slash + 1;
        wcsncpy_s(wszFile, MAX_PATH, base, _TRUNCATE);
    }

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFile       = wszFile;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFilter     = isColormap ? filterPpm : filterPgm;
    ofn.nFilterIndex    = 1;
    ofn.lpstrDefExt     = isColormap ? L"ppm" : L"pgm";
    ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    // 用户取消或出错
    if (!GetSaveFileNameW(&ofn)) {
        return std::string();
    }

    // 把选中的宽字符路径转回UTF-8
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wszFile, -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(ulen), 0);
    WideCharToMultiByte(CP_UTF8, 0, wszFile, -1, result.data(), ulen, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') result.pop_back();   // 去掉末尾 \0
    return result;
}

}
