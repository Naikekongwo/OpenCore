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
     * @brief 常规纹理构造器
     *
     * @param x
     * @param y
     * @param tex
     */
    Texture(size_t x, size_t y, std::shared_ptr<SDL_Texture> tex);

    /**
     * @brief 仅指定网格尺寸的占位构造器（不传入 SDL_Texture）
     *        适用于异步加载场景：先创建占位纹理，待 SDL_Texture 加载完成后
     *        再通过 configure() 设置。
     *
     * @param x 列数
     * @param y 行数
     */
    Texture(size_t x, size_t y);

    /**
     * @brief 构造带资源名的占位纹理。
     *        内部 SDL_Texture 为 nullptr，首次调用 get() 时会按 name
     *        从 PackageManager 懒加载真实的 SDL_Texture。
     *
     * @param x    列数
     * @param y    行数
     * @param name 纹理资源名称（用于后续懒加载）
     */
    Texture(size_t x, size_t y, std::string_view name);

    /**
     * @brief 构造空纹理（xCount = yCount = 1，无 SDL_Texture）
     *
     */
    Texture();

    // 行数和列数
    size_t xCount = 1;
    size_t yCount = 1;

    // 贴图的大小
    uint16_t width, height;

    // 材质
    std::shared_ptr<SDL_Texture> texture;

    // 资源名称（用于懒加载，为空时不触发自动加载）
    std::string name;

    /**
     * @brief 获取 SDL_Texture 指针。
     *        若内部 texture 为空且 name 非空，则尝试从 PackageManager
     *        按 name 懒加载。加载成功则缓存并返回，否则返回 nullptr。
     */
    SDL_Texture *get();

    // 隐式转换为 SDL_Texture*（调用 get()，含懒加载）
    operator SDL_Texture *() { return get(); }

    uint16_t Size() const noexcept { return xCount * yCount; }

    float getWidthHeight() const noexcept
    {
        return static_cast<float>(width) / static_cast<float>(height);
    }

    SDL_Rect getSubRect(size_t index);
    Rect     getSubRect(size_t startIndex, size_t endIndex);
    /// @brief 以 startIndex 为左上角，提取 cols 列 × rows 行的子矩形
    Rect getSubRect(size_t startIndex, uint8_t cols, uint8_t rows);

    /**
     * @brief 重新设置纹理属性
     *
     * @param rows
     * @param cols
     * @param texture
     * @return true
     * @return false
     */
    bool configure(size_t rows, size_t cols,
                   std::shared_ptr<SDL_Texture> texture);
};
