/**
 * @file ImageBoard.hpp
 * @brief 基础图片显示控件。
 *
 * 提供简单的图像显示功能，继承自 UIElement，支持纹理帧索引、透明度、
 * 磁吸边距等基础视觉特性。
 */

#ifndef _IMAGEBOARD_H_
#define _IMAGEBOARD_H_

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include <cstddef>
#include <string>

namespace OpenUI
{

/**
 * @class ImageBoard
 * @brief 静态图片或单帧动画的显示控件。
 *
 * 用于显示单个纹理（或纹理中的某一帧），不包含交互逻辑。
 * 支持通过 VisualState 控制透明度、旋转角度、帧索引等。
 * UIElement 的圆角半径在这里会作为内容蒙版使用。
 */
class ImageBoard : public UIElement
{
  public:
    /**
     * @brief 构造 ImageBoard 对象。
     * @param id      唯一标识符。
     * @param layer   渲染图层（值越小越靠后）。
     * @param texture 关联的纹理智能指针（不能为空）。
     */
    ImageBoard(std::string_view id, uint8_t layer, shared_ptr<Texture> texture);

    void Draw() override;
    void onUpdate(float totalTime) override;
    bool onDestroy() override;

  private:
    void ensureMaskedTexture(const SDL_Rect &bounds, float radiusPx,
                             float borderPx);

    shared_ptr<Texture> m_maskCache;      ///< 圆角蒙版缓存
    shared_ptr<Texture> m_compositeCache; ///< 内容合成缓存

    Texture *m_maskedSource = nullptr; ///< 合成时的源纹理
    size_t   m_maskedFrame  = 0;       ///< 合成时的帧
    int      m_maskedWidth  = 0;       ///< 合成纹理宽
    int      m_maskedHeight = 0;       ///< 合成纹理高
    float    m_maskedRadius = 0.0f;    ///< 合成时的圆角（逻辑像素）
    float    m_maskedBorder = 0.0f;    ///< 合成时的描边（逻辑像素）

    int   m_maskWidth  = 0;    ///< 蒙版缓存宽
    int   m_maskHeight = 0;    ///< 蒙版缓存高
    float m_maskRadius = 0.0f; ///< 蒙版缓存圆角
    float m_maskBorder = 0.0f; ///< 蒙版缓存描边
};

} // namespace OpenUI

#endif //_IMAGEBOARD_H_