#pragma once

#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

class Character : public UIElement
{
};

using CharaPtr = unique_ptr<Character>;