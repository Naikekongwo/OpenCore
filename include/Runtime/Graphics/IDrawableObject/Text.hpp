#pragma once

#include "Core/Math/OpenCore_Color.hpp"
#include "Core/Math/OpenCore_Rect.hpp"

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
    RENDER_GRADIENT = 1 << 1
};

/// 字体信息 + 渲染选项
struct TextAttribute
{
    Color            color    = White; ///< 文本颜色（无渐变时生效）
    int              fontSize = 36;
    string           fontName = "OpenCoreFont";
    TextRenderOption option   = RENDER_TEXT; ///< 渲染层标志
    Color gradientColor = None; ///< 渐变色（顶部透明 → 底部 gradientColor）
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