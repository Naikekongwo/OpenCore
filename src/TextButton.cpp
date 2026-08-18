#include "Runtime/Graphics/UI/TextButton.hpp"
#include "Core/Math/OpenCore_Color.hpp"
#include "Core/Math/OpenCore_Rect.hpp"
#include "OpenCore.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"

#include <SDL3/SDL_render.h>
#include <memory>

TextButton::TextButton(std::string_view id, uint8_t layer, string_view text)
    : UIElement(id, layer, nullptr), m_textContent(text)
{
}

void TextButton::onClick(Event *event, const SDL_Point &mousePos)
{
    if (m_onClickCallback)
        m_onClickCallback();
}

void TextButton::onUpdate(float totalTime)
{
    if (!isAnimeFinished())
    {
        IDrawableObject::onUpdate(totalTime);
    }

    if (m_textureDirty)
    {
        rebuildTexture();
    }
}

void TextButton::rebuildTexture()
{
    m_textureCache.reset();

    SDL_Rect bounds = getLogicalBounds();

    // 布局尚未完成（尺寸为 0）时保留 dirty，待布局完成后再重建
    if (bounds.w <= 0 || bounds.h <= 0)
    {
        LOG("TextButton({}) 逻辑尺寸非法 ({}x{})，等待布局完成后重建", id,
            bounds.w, bounds.h);
        return;
    }

    // TextButton 需要 3 帧（Normal / Hovered / Pressed）竖直排列
    m_textureCache = std::make_shared<Texture>(static_cast<uint16_t>(bounds.w),
                                               static_cast<uint16_t>(bounds.h),
                                               size_t(1), size_t(3));

    if (m_textureCache && m_textureCache->get())
    {
        generateTexture();
        m_textureDirty = false;
    }
    else
    {
        LOG("m_textureCache 创建失败 ({}x{} 3帧)", bounds.w, bounds.h);
    }
}

void TextButton::Draw()
{
    UIElement::Draw();

    if (m_textContent.empty())
        return;

    Rect dstRect = getLogicalBounds();
    Rect VRect   = OpenCoreManagers::GFXManager.getInstance().getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    if (!m_textureCache || !m_textureCache->get())
        return;

    // 直接以交互状态驱动三态帧，避免污染动画帧索引（VState->frameIndex）
    int stateIndex = static_cast<int>(m_interactionState);
    if (static_cast<size_t>(stateIndex) >= m_textureCache->Size())
        stateIndex = 0;

    Rect srcRectText = m_textureCache->getSubRect(stateIndex);

    m_textureCache->Draw(&srcRectText, &dstRect, VState->getAngle(), nullptr,
                         static_cast<uint8_t>(VState->getAlpha()));
}

bool TextButton::generateTexture()
{
    if (m_textContent.empty() || m_textContent == " ")
    {
        LOG("文字内容为空，跳过");
        return false;
    }

    Rect bounds = getLogicalBounds();

    if (bounds.h <= 0)
    {
        LOG("控件高度非法: h={}", bounds.h);
        return false;
    }

    if (bounds.w <= 0)
    {
        LOG("控件宽度非法: w={}", bounds.w);
        return false;
    }

#pragma region 测量并生成文字纹理
    // 共享文字属性基准（测量 + 三帧共用）。字号按按钮高度自适应，
    // 其余属性以 normal 属性为基准，悬停/按下仅覆盖颜色相关字段。
    TextAttribute baseAttr = m_normalAttribute;
    baseAttr.fontName      = m_normalAttribute.fontName;
    baseAttr.fontSize      = bounds.h * 0.9f;
    // 渲染选项、描边宽度、阴影偏移等均以 normal 属性为准，
    // 允许调用方通过 setNormalAttribute 自定义（不再硬编码覆盖）

    // 测量文字尺寸
    int W = 0, H = 0;
    Text::Measure(m_textContent, baseAttr, W, H);

    if (W * H == 0)
    {
        LOG("文字测量面积为0");
        return false;
    }

    // 按按钮尺寸缩放字号：文字行高（含行距）超出按钮高度、或宽度超出时，
    // 按比例缩小字号重新测量，使文字完整放入三帧、不被裁剪（按高度缩放）
    if (W > bounds.w || H > bounds.h)
    {
        float scaleX = bounds.w / static_cast<float>(W);
        float scaleY = bounds.h / static_cast<float>(H);
        float scale  = scaleX < scaleY ? scaleX : scaleY;

        baseAttr.fontSize = static_cast<int>(baseAttr.fontSize * scale);
        if (baseAttr.fontSize < 1)
            baseAttr.fontSize = 1;

        Text::Measure(m_textContent, baseAttr, W, H);
        if (W * H == 0)
        {
            LOG("文字测量面积为0（缩放字号后）");
            return false;
        }
    }

    // 文字层宽度取文字实际像素宽度（不超过按钮宽度）
    float textLayerBoundsWidth = static_cast<float>(W);
    if (textLayerBoundsWidth > bounds.w)
        textLayerBoundsWidth = bounds.w;

    if (textLayerBoundsWidth <= 0.0f)
    {
        LOG("textLayerBoundsWidth 非法: {}", textLayerBoundsWidth);
        return false;
    }

    // 构造离屏 3 帧纹理
    auto textLayer = std::make_shared<Texture>(
        static_cast<uint16_t>(textLayerBoundsWidth),
        static_cast<uint16_t>(bounds.h), size_t(1), size_t(3));

    if (!textLayer || !textLayer->get())
    {
        LOG("textLayer 创建失败 ({}x{} 3帧)",
            static_cast<uint16_t>(textLayerBoundsWidth),
            static_cast<uint16_t>(bounds.h));
        return false;
    }

    // 逐帧渲染 Normal / Hovered / Pressed
    const TextAttribute *stateAttrs[3] = {
        &m_normalAttribute, &m_hoveredAttribute, &m_pressedAttribute};

    // 帧内文字垂直对齐（上 / 中 / 下），由 m_textAlign 决定
    float yOffset = 0.0f;
    switch (m_textAlign)
    {
    case AnchorPoint::MiddleLeft:
    case AnchorPoint::Center:
    case AnchorPoint::MiddleRight:
        yOffset = (bounds.h - H) * 0.5f;
        break;
    case AnchorPoint::BottomLeft:
    case AnchorPoint::BottomCenter:
    case AnchorPoint::BottomRight:
        yOffset = bounds.h - H;
        break;
    default: // Top 系列
        yOffset = 0.0f;
        break;
    }
    if (yOffset < 0.0f)
        yOffset = 0.0f;

    for (int i = 0; i < 3; i++)
    {
        Rect rect    = textLayer->getSubRect(i);
        Rect dstRect = {(rect.w - W) * 0.5f, yOffset, static_cast<float>(W),
                        static_cast<float>(H)};

        // 从状态属性中只覆盖颜色字段，其余从 baseAttr 继承
        TextAttribute attr = baseAttr;
        attr.color         = stateAttrs[i]->color;
        attr.borderColor   = stateAttrs[i]->borderColor;
        attr.glowColor     = stateAttrs[i]->glowColor;
        attr.gradientColor = stateAttrs[i]->gradientColor;
        Text::Draw(textLayer.get(), &dstRect, m_textContent, attr);

        yOffset += textLayer->height;
    }

#pragma endregion

    // 文字贴图水平对齐（左 / 中 / 右），由 m_textAlign 决定
    float offsetX = 0.0f;
    switch (m_textAlign)
    {
    case AnchorPoint::TopLeft:
    case AnchorPoint::MiddleLeft:
    case AnchorPoint::BottomLeft:
        offsetX = 0.0f;
        break;
    case AnchorPoint::TopCenter:
    case AnchorPoint::Center:
    case AnchorPoint::BottomCenter:
        offsetX = (bounds.w - textLayerBoundsWidth) * 0.5f;
        break;
    case AnchorPoint::TopRight:
    case AnchorPoint::MiddleRight:
    case AnchorPoint::BottomRight:
        offsetX = bounds.w - textLayerBoundsWidth;
        break;
    }

    // 防止文本宽度超出时偏移出界（退化为左对齐）
    if (offsetX < 0.0f)
        offsetX = 0.0f;

    // 检查目标纹理是否有效
    if (!m_textureCache || !m_textureCache->get())
    {
        LOG("目标纹理 m_textureCache 无效");
        return false;
    }

    // 逐帧合成：利用 Texture::Draw 将 textLayer 绘制到 m_textureCache
    // Texture::Draw 内部自动管理 RenderTarget 切换，无需手动操作
    for (int i = 0; i < 3; i++)
    {
        SDL_Rect rawSrc  = textLayer->getSubRect(i);
        Rect     srcRect = rawSrc;
        Rect     dstRect = {offsetX, static_cast<float>(i) * bounds.h,
                            textLayerBoundsWidth, bounds.h};
        textLayer->Draw(m_textureCache.get(), &srcRect, &dstRect, 0.0, nullptr,
                        255);
    }

    return true;
}
