#pragma once

#include "Core/Math/OpenCore_Color.hpp"
#include "Core/Math/OpenCore_Rect.hpp"
#include "Core/Math/OpenCore_Vec2.hpp"

#include <memory>
#include <string>

using std::shared_ptr;
using std::string;
using std::string_view;

struct Texture;

enum TextRenderOption
{
    RENDER_TEXT     = 0,
    RENDER_SHADOW   = 1 << 0, // 0x01
    RENDER_GRADIENT = 1 << 1,
    RENDER_GLOW     = 1 << 2,
    RENDER_BORDER   = 1 << 3,
};

/// 字体信息 + 渲染选项
struct TextAttribute
{

    TextRenderOption option = RENDER_TEXT; ///< 渲染层标志

    Color color         = White; ///< 文本颜色（无渐变时生效）
    Color gradientColor = None;  ///< 渐变色（顶部透明 → 底部 gradientColor）
    Color glowColor     = None;  /// <外发光色>
    Color borderColor   = Black;

    int    fontSize   = 36;             /// <字体大小>
    string fontName   = "OpenCoreFont"; /// <字体名称>
    int    BorderSize = 10;             /// <描边像素>

    bool shadowGradient = true;        /// <阴影边缘柔化>
    Vec2 shadowOffset{0.005f, 0.005f}; /// <阴影偏移>
};

class Text
{
  public:
    /// @brief 测量文本尺寸（像素）。
    static bool Measure(string_view textContent, const TextAttribute &attr,
                        int &outW, int &outH);

    /// @brief 将文本渲染到目标纹理上。
    static void Draw(Texture *target, const Rect *dstRect,
                     string_view textContent, const TextAttribute &attr);
};