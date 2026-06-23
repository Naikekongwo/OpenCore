#pragma once

#include <SDL3/SDL.h>
#include <algorithm>

struct OpenCore_Color
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;

    OpenCore_Color() = default;

    OpenCore_Color(float _r, float _g, float _b, float _a)
        : r(_r), g(_g), b(_b), a(_a)
    {
    }

    operator SDL_Color() const noexcept
    {
        auto toU8 = [](float v) -> uint8_t
        { return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f); };
        return {toU8(r), toU8(g), toU8(b), toU8(a)};
    }
};

using Color = OpenCore_Color;

static const Color White = Color(1.0f, 1.0f, 1.0f, 1.0f);
static const Color Black = Color(0.0f, 0.0f, 0.0f, 1.0f);
static const Color None  = Color(0.0f, 0.0f, 0.0f, 0.0f);