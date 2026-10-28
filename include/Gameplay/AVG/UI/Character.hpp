#pragma once

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

class Character : public OpenUI::UIElement
{
};

using CharaPtr = unique_ptr<Character>;