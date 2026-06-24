/**
 * @file Button.hpp
 * @brief 按钮 UI 控件。
 *
 * 提供可交互的按钮组件，支持三种状态（普通、悬停、按下），
 * 可绑定点击回调函数，并根据状态显示纹理的不同帧。
 *
 * @note 交互状态管理由基类 UIElement 统一处理，Button 只需关注
 *       点击回调触发与纹理帧映射。
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "Core/Math/OpenCore_Color.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

#include <SDL3/SDL_render.h>
#include <functional>
#include <memory>

enum class ButtonStyle
{
    Image,
    Text,
    Hybrid
};

#pragma region 按钮字体的默认状态
// 正常状态：文字白色亮丽，黑色渐变，白色外发光
static const TextAttribute normal_attr{
    static_cast<TextRenderOption>(RENDER_TEXT | RENDER_BORDER | RENDER_GLOW |
                                  RENDER_SHADOW | RENDER_GRADIENT),
    White,
    Black,
    White,
    Black,
    36,
    "OpenCoreFont",
    2,
    true,
    {2, 2}};
// 悬浮状态：亮度降低，整体变灰
static const TextAttribute hovered_attr{
    static_cast<TextRenderOption>(RENDER_TEXT | RENDER_BORDER | RENDER_GLOW |
                                  RENDER_SHADOW | RENDER_GRADIENT),
    Color(0.65f, 0.65f, 0.65f, 1.0f),
    Color(0.30f, 0.30f, 0.30f, 1.0f),
    Color(0.65f, 0.65f, 0.65f, 1.0f),
    Color(0.30f, 0.30f, 0.30f, 1.0f),
    36,
    "OpenCoreFont",
    2,
    true,
    {2, 2}};
// 按下状态：颜色反转，文字黑色，渐变白色，外发光黑色
static const TextAttribute pressed_attr{
    static_cast<TextRenderOption>(RENDER_TEXT | RENDER_BORDER | RENDER_GLOW |
                                  RENDER_SHADOW | RENDER_GRADIENT),
    Black,
    White,
    Black,
    White,
    36,
    "OpenCoreFont",
    2,
    true,
    {2, 2}};
#pragma endregion

/**
 * @class Button
 * @brief 可点击的按钮控件。
 *
 * 继承自 UIElement，利用基类的 InteractionState 自动管理鼠标交互状态，
 * 根据状态自动切换纹理帧（纹理需包含三帧，按 Normal、Hovered、Pressed
 * 顺序排列）。支持绑定点击回调函数。
 */
class Button : public UIElement
{
  public:
    /**
     * @brief 构造按钮对象。
     * @param id      唯一标识符。
     * @param layer   渲染图层。
     * @param texture 按钮纹理（应包含至少三帧，顺序为 Normal, Hovered,
     * Pressed）。
     */
    Button(const std::string &id, uint8_t layer, shared_ptr<Texture> texture);

    void Draw() override;
    void onUpdate(float totalTime) override;

    /**
     * @brief 设置按钮点击时的回调函数。
     * @param func 无参无返回值的回调函数。
     */
    void setOnClick(std::function<void()> func)
    {
        m_onClickCallback = std::move(func);
    }

    void setButtonStyle(ButtonStyle style)
    {
        m_buttonstyle  = style;
        m_textureDirty = true;
    }
    void setButtonText(string_view text)
    {
        m_textContent  = text;
        m_textureDirty = true;
    }

    void setNormalAttribute(const TextAttribute &attr)
    {
        normal_attribute = attr;
        m_textureDirty   = true;
    }
    void setHoveredAttribute(const TextAttribute &attr)
    {
        hovered_attribute = attr;
        m_textureDirty    = true;
    }
    void setPressedAttribute(const TextAttribute &attr)
    {
        pressed_attribute = attr;
        m_textureDirty    = true;
    }

    bool generateTexture() override;

  protected:
    void onClick(Event *event, const SDL_Point &mousePos) override;

  private:
    /** @brief 获取可用帧数（优先 m_textureCache，退回到基类 texture） */
    size_t getMaxFrames() const;

    string      m_textContent;
    ButtonStyle m_buttonstyle = ButtonStyle::Image; /// <按钮风格>

    std::function<void()> m_onClickCallback; ///< 点击回调函数

    TextAttribute normal_attribute  = normal_attr;
    TextAttribute hovered_attribute = hovered_attr;
    TextAttribute pressed_attribute = pressed_attr;
};

#endif //_BUTTON_H_