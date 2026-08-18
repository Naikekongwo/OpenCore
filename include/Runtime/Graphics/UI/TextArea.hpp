#pragma once

#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

#include <memory>
#include <string>

using std::string;
using std::unique_ptr;

class TextArea : public UIElement
{
  public:
    TextArea(std::string_view id, uint8_t layer, std::string_view fontName);

    void parseEvents(Event *event, float totalTime) override;
    void onUpdate(float totalTime) override;
    void Draw() override;

    void setText(string_view textContent);
    void setFontSize(short fontSize);
    void setShadow(bool enableTag, int shadowOffset);

    /**
     * @brief 设置文字对齐方式（九宫格：水平 × 垂直，沿用 AnchorPoint）。
     * @param align 对齐锚点（默认 TopLeft，与旧版不居中对齐行为一致）。
     */
    void align(AnchorPoint align)
    {
        m_align        = align;
        m_textureDirty = true;
    }

    /**
     * @brief 便捷方法：是否水平垂直居中。
     * @deprecated 请使用 align(AnchorPoint::Center)。
     */
    void alignCenter(bool tag)
    {
        m_align = tag ? AnchorPoint::Center : AnchorPoint::TopLeft;
    }

    void setTextColor(uint8_t r, uint8_t g, uint8_t b)
    {
        m_textAttr.color = Color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        m_textureDirty   = true;
    }

    /**
     * @brief 直接设置完整文字属性（字体、渲染选项、颜色、渐变、描边、阴影）。
     * @note 之后调用 setFontSize() 可单独覆盖字号。
     */
    void setAttribute(const TextAttribute &attr)
    {
        m_textAttr     = attr;
        m_textureDirty = true;
    }

  private:
    bool generateTexture() override;

    TextAttribute m_textAttr;
    string        m_textContent;
    AnchorPoint   m_align = AnchorPoint::TopLeft; ///< 文字对齐方式（默认左上）

    float transparency = 0.69f; ///< 阴影透明度系数
};