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
    /** @brief 构造纹理。内部 SDL_Texture 必须不为空。 */
    Texture(size_t x, size_t y, std::shared_ptr<SDL_Texture> tex);

    /** @brief 构造离屏纹理，向 GraphicsManager 请求 SDL_Texture。 */
    Texture(uint16_t frameW, uint16_t frameH, size_t x, size_t y);

    std::shared_ptr<SDL_Texture> texture;    ///< 材质
    size_t                       xCount = 1; ///< 列数（网格分割）
    size_t                       yCount = 1; ///< 行数
    uint16_t                     width  = 0; ///< 单帧宽度
    uint16_t                     height = 0; ///< 单帧高度

    operator SDL_Texture *();

    SDL_Texture *get() const;

    void     QueryTexture(int &w, int &h) const;
    void     QueryRawTexture(int &w, int &h) const;
    float    getTextureRatio() const noexcept;
    uint16_t Size() const noexcept;
    SDL_Rect getSubRect(size_t index);

    int Draw(const Rect *srcRect, const Rect *dstRect, double angle = 0.0,
             const Point *center = nullptr, uint8_t alpha = 255);

    /** @brief 绘制到另一个纹理（离屏渲染）。内部切换 RenderTarget。 */
    int Draw(Texture *target, const Rect *srcRect, const Rect *dstRect,
             double angle = 0.0, const Point *center = nullptr,
             uint8_t alpha = 255);
};
