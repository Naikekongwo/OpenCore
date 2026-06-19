#pragma once

#include "Core/Math/OpenCore_Rect.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <string>
using std::shared_ptr;
using std::string;
using std::unique_ptr;

struct Texture
{
    /**
     * @brief 构造纹理。
     *        内部 SDL_Texture 必须不为空 —— 调用方需保证已通过
     *        PackageManager::getTextureAsync 同步加载完成。
     *
     * @param x   列数（网格分割）
     * @param y   行数
     * @param tex SDL_Texture 智能指针
     */
    Texture(size_t x, size_t y, std::shared_ptr<SDL_Texture> tex);

    // 行数和列数
    size_t xCount = 1;
    size_t yCount = 1;

    // 贴图的大小（单帧尺寸）
    uint16_t width, height;

    // 材质
    std::shared_ptr<SDL_Texture> texture;

    /**
     * @brief 获取 SDL_Texture 指针。
     */
    SDL_Texture *get() const { return texture.get(); }

    // 隐式转换为 SDL_Texture*
    operator SDL_Texture *() { return texture.get(); }

    uint16_t Size() const noexcept { return xCount * yCount; }

    float getWidthHeight() const noexcept
    {
        return static_cast<float>(width) / static_cast<float>(height);
    }

    SDL_Rect getSubRect(size_t index);
    Rect getSubRect(size_t startIndex, size_t endIndex);
    /// @brief 以 startIndex 为左上角，提取 cols 列 × rows 行的子矩形
    Rect getSubRect(size_t startIndex, uint8_t cols, uint8_t rows);
};
