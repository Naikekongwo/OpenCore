/**
 * @file RoundedRect.hpp
 * @brief 圆角矩形的抗锯齿离屏生成工具。
 *
 * 供 UIElement 的背景/描边与 ImageBoard 的内容蒙版共用。半径与描边宽度
 * 都是逻辑像素，换算由调用方负责。
 */
#pragma once

#include "Core/Math/OpenCore_Color.hpp"
#include "Runtime/Graphics/IDrawableObject/Texture.hpp"

#include <SDL3/SDL_rect.h>

#include <memory>

namespace OpenUI
{

class RoundedRect
{
  public:
    struct Style
    {
        Color fill        = None;
        Color border      = None;
        float borderWidth = 0.0f; ///< 逻辑像素
        float radius      = 0.0f; ///< 逻辑像素
    };

    /**
     * @brief 生成带抗锯齿边缘的圆角矩形纹理（填充 + 描边）
     * @return 失败返回 nullptr
     */
    static std::shared_ptr<Texture> Build(const SDL_Rect &bounds,
                                          const Style    &style);

    /**
     * @brief 生成白色 + 覆盖率的圆角蒙版，内容按描边宽度内缩后裁剪
     * @return 失败返回 nullptr
     */
    static std::shared_ptr<Texture> BuildMask(const SDL_Rect &bounds,
                                              float radius, float borderWidth);
};

} // namespace OpenUI
