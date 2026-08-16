/**
 * @file DrawableConfigurator.hpp
 * @author Naikekongwo
 * @brief
 * @version 1.0
 * @date 2025-08-07
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef _DRAWABLECONFIGURATOR_H_
#define _DRAWABLECONFIGURATOR_H_

#include <cstdint>

enum class AnchorPoint : int;

class IDrawableObject;

class DrawableConfigurator
{
  public:
    // 构造函数
    DrawableConfigurator(IDrawableObject *obj) : obj_p(obj) {}

    DrawableConfigurator &Scale(float w, float h);

    /**
     * @brief 强制按相对尺寸缩放（不做绝对像素自动判断）。
     *        w/h 直接作为父容器尺寸的倍数，即使 > 1.0f 也保持相对语义。
     * @param w 宽度（父容器宽度的倍数，0 表示按纹理宽高比自动计算）。
     * @param h 高度（父容器高度的倍数，0 表示按纹理宽高比自动计算）。
     */
    DrawableConfigurator &ScaleR(float w, float h);

    DrawableConfigurator &Anchor(AnchorPoint anchor);
    DrawableConfigurator &Posite(float xPercent, float yPercent);

    DrawableConfigurator &Sequence(bool sequential);

    DrawableConfigurator &Parent(IDrawableObject *parent);

    DrawableConfigurator &Alpha(float alphaValue);

    DrawableConfigurator &Follow(int margin);

  private:
    IDrawableObject *obj_p;
};

#endif //_DRAWABLECONFIGURATOR_H_