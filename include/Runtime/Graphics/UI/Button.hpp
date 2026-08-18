/**
 * @file Button.hpp
 * @brief 图片按钮 UI 控件。
 *
 * 提供基于外部纹理的可交互按钮组件，支持三种状态（普通、悬停、按下），
 * 可绑定点击回调函数，并根据交互状态显示纹理的不同帧。
 *
 * @note 交互状态管理由基类 UIElement 统一处理，Button 只需关注
 *       点击回调触发与纹理帧映射。
 * @note 实时渲染文字按钮请使用 TextButton
 *       （Runtime/Graphics/UI/TextButton.hpp）。
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "Core/Math/OpenCore_Color.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

#include <SDL3/SDL_render.h>
#include <functional>
#include <memory>

/**
 * @class Button
 * @brief 可点击的图片按钮控件。
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
    Button(std::string_view id, uint8_t layer, shared_ptr<Texture> texture);

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

  protected:
    void onClick(Event *event, const SDL_Point &mousePos) override;

  private:
    /** @brief 获取可用帧数（基于按钮纹理） */
    size_t getMaxFrames() const;

    std::function<void()> m_onClickCallback; ///< 点击回调函数
};

#endif //_BUTTON_H_