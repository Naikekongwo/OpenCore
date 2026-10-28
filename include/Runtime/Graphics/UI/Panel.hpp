/**
 * @file Panel.hpp
 * @brief 纯背景面板控件（无纹理、无内容）。
 *
 * 背景填充、圆角与描边全部由 UIElement 提供；Panel 只负责把
 * 「一个没有内容的控件」这层语义表达出来。
 */
#pragma once

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

namespace OpenUI
{

class Panel : public UIElement
{
  public:
    Panel(std::string_view id, uint8_t layer);

    void onUpdate(float totalTime) override;
};

} // namespace OpenUI
