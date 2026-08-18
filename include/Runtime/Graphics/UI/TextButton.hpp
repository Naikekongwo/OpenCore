/**
 * @file TextButton.hpp
 * @brief 实时渲染文字按钮 UI 控件。
 *
 * 与基于外部纹理的图片 Button 不同，TextButton 不依赖任何图片资源：
 * 文字会在运行时根据按钮实际尺寸实时测量并渲染为离屏纹理，同时维护
 * Normal / Hovered / Pressed 三套文字属性（颜色、外发光、渐变等）。
 *
 * @note 交互状态管理由基类 UIElement 统一处理，TextButton 直接以
 *       InteractionState 驱动三态帧的绘制，不参与 VState->frameIndex
 *       动画帧索引系统（避免与动画系统耦合）。
 */

#ifndef _TEXTBUTTON_H_
#define _TEXTBUTTON_H_

#include "Core/Math/OpenCore_Color.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

#include <SDL3/SDL_render.h>
#include <functional>
#include <memory>

#pragma region 按钮字体的默认状态
// 正常状态：文字白色亮丽，黑色渐变，白色外发光
inline const TextAttribute kTextButtonNormalAttr{
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
inline const TextAttribute kTextButtonHoveredAttr{
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
inline const TextAttribute kTextButtonPressedAttr{
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
 * @class TextButton
 * @brief 可点击的实时渲染文字按钮控件。
 *
 * 继承自 UIElement，利用基类的 InteractionState 自动管理鼠标交互状态。
 * 文字被预渲染为 3 帧离屏纹理（Normal / Hovered / Pressed），绘制时按
 * 当前交互状态选择对应帧，无需每次重绘文字。
 */
class TextButton : public UIElement
{
  public:
    /**
     * @brief 构造文字按钮对象。
     * @param id    唯一标识符。
     * @param layer 渲染图层。
     * @param text  按钮文字（可在运行时通过 setText 修改）。
     */
    TextButton(std::string_view id, uint8_t layer, string_view text = "");

    void Draw() override;
    void onUpdate(float totalTime) override;

    /**
     * @brief 设置按钮文字，触发离屏纹理重建。
     * @param text 新的按钮文字。
     */
    void setText(string_view text)
    {
        m_textContent  = text;
        m_textureDirty = true;
    }

    /**
     * @brief 设置按钮点击时的回调函数。
     * @param func 无参无返回值的回调函数。
     */
    void setOnClick(std::function<void()> func)
    {
        m_onClickCallback = std::move(func);
    }

    /**
     * @brief 设置正常状态文字属性（渲染选项、颜色、发光、渐变等）。
     * @note 渲染选项（option）会全局作用于三个状态；
     *       字号不在此生效——文字尺寸始终按按钮实际高度自适应。
     */
    void setNormalAttribute(const TextAttribute &attr)
    {
        m_normalAttribute = attr;
        m_textureDirty    = true;
    }
    /**
     * @brief 设置悬浮状态文字属性。
     */
    void setHoveredAttribute(const TextAttribute &attr)
    {
        m_hoveredAttribute = attr;
        m_textureDirty     = true;
    }
    /**
     * @brief 设置按下状态文字属性。
     */
    void setPressedAttribute(const TextAttribute &attr)
    {
        m_pressedAttribute = attr;
        m_textureDirty     = true;
    }

    /**
     * @brief 设置按钮内部文字贴图的对齐方式（水平 × 垂直，沿用 AnchorPoint）。
     * @param align 对齐锚点（默认 Center）。
     */
    void align(AnchorPoint align)
    {
        m_textAlign    = align;
        m_textureDirty = true;
    }

    bool generateTexture() override;

  protected:
    void onClick(Event *event, const SDL_Point &mousePos) override;

  private:
    /**
     * @brief 重建 3 帧离屏文字纹理。
     * @note 布局尚未完成（逻辑尺寸为 0）时保留 dirty 标志，待后续
     *       onUpdate 自动重试，避免纹理永远无法生成。
     */
    void rebuildTexture();

    string m_textContent; ///< 按钮文字

    std::function<void()> m_onClickCallback; ///< 点击回调函数

    TextAttribute m_normalAttribute  = kTextButtonNormalAttr;
    TextAttribute m_hoveredAttribute = kTextButtonHoveredAttr;
    TextAttribute m_pressedAttribute = kTextButtonPressedAttr;

    AnchorPoint m_textAlign = AnchorPoint::Center; ///< 文字对齐方式（默认居中）
};

#endif //_TEXTBUTTON_H_
