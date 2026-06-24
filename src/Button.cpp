
#include "Runtime/Graphics/UI/Button.hpp"
#include "Core/Math/OpenCore_Color.hpp"
#include "Core/Math/OpenCore_Rect.hpp"
#include "OpenCore.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include <SDL3/SDL_render.h>
#include <memory>

Button::Button(const std::string &id, uint8_t layer,
               shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
}

size_t Button::getMaxFrames() const
{
    if (m_textureCache && m_textureCache->get())
        return m_textureCache->Size();
    if (texture && texture->get())
        return texture->Size();
    return 0;
}

void Button::onClick(Event *event, const SDL_Point &mousePos)
{
    if (m_onClickCallback)
        m_onClickCallback();
}

void Button::onUpdate(float totalTime)
{
    if (!isAnimeFinished())
    {
        IDrawableObject::onUpdate(totalTime);
    }

    if (m_textureDirty)
    {
        m_textureCache.reset();
        SDL_Rect bounds = getLogicalBounds();

        if (bounds.w == 0 || bounds.h == 0)
        {
            LOG("bounds 尺寸为 0×0，跳过纹理创建");
            m_textureDirty = false;
            return;
        }

        // Button 需要 3 帧（Normal / Hovered / Pressed）竖直排列
        m_textureCache = std::make_shared<Texture>(
            static_cast<uint16_t>(bounds.w), static_cast<uint16_t>(bounds.h),
            size_t(1), size_t(3));

        if (m_textureCache && m_textureCache->get())
        {
            generateTexture();
        }
        else
        {
            LOG("m_textureCache 创建失败 ({}x{} 3帧)", bounds.w, bounds.h);
        }
        m_textureDirty = false;
    }

    size_t maxFrames = getMaxFrames();
    if (maxFrames == 0)
        return;

    int stateIndex = static_cast<int>(m_interactionState);

    if (static_cast<size_t>(stateIndex) >= maxFrames)
        stateIndex = 0;

    VState->frameIndex = stateIndex;
}

void Button::Draw()
{
    UIElement::Draw();

    Rect dstRect = getLogicalBounds();
    Rect VRect   = OpenCoreManagers::GFXManager.getInstance().getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    size_t maxFrames = getMaxFrames();
    if (maxFrames == 0)
        return;

    auto frameIndex =
        (VState->getFrameIndex() > maxFrames) ? 0 : VState->getFrameIndex();

    // 绘制基座纹理（Image / Hybrid 模式使用）
    if (texture && texture->get())
    {
        Rect srcRect = texture->getSubRect(frameIndex);
        texture->Draw(&srcRect, &dstRect, VState->getAngle(), nullptr,
                      static_cast<uint8_t>(VState->getAlpha()));
    }

    /// <绘制文字层（Text / Hybrid 模式）>
    if (m_buttonstyle == ButtonStyle::Image)
        return;

    if (!m_textureCache || !m_textureCache->get())
        return;

    Rect srcRectText = m_textureCache->getSubRect(frameIndex);

    m_textureCache->Draw(&srcRectText, &dstRect, VState->getAngle(), nullptr,
                         static_cast<uint8_t>(VState->getAlpha()));
}

bool Button::generateTexture()
{
    /// 纹理的重建只会出现在需要文字的按钮风格
    if (m_buttonstyle == ButtonStyle::Image)
        return true;

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

    /// 构建共享文字属性基准（测量 + 三帧共用）
    TextAttribute baseAttr = normal_attribute;
    baseAttr.fontName      = "Font_Eng";
    baseAttr.fontSize      = bounds.h * 0.9f;
    baseAttr.BorderSize    = 1;
    baseAttr.option        = static_cast<TextRenderOption>(
        RENDER_TEXT | RENDER_SHADOW | RENDER_GRADIENT | RENDER_BORDER |
        RENDER_GLOW);
    baseAttr.shadowOffset   = {2, 2};
    baseAttr.shadowGradient = true;

    /// 测量文字尺寸
    int W = 0, H = 0;
    Text::Measure(m_textContent, baseAttr, W, H);

    if (W * H == 0)
    {
        LOG("文字测量面积为0");
        return false;
    }

    /// 通过计算的W,H，得到纹理的大小(以H优先，W尽力拓展即可
    float ratio                = (W * 1.0f) / H;
    float textLayerBoundsWidth = ratio * bounds.h;

    if (textLayerBoundsWidth <= 0.0f)
    {
        LOG("textLayerBoundsWidth 非法: {}", textLayerBoundsWidth);
        return false;
    }

    /// 构造纹理（离屏 3 帧）
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

    /// 循环渲染三帧：Normal / Hovered / Pressed
    const TextAttribute *stateAttrs[3] = {&normal_attribute, &hovered_attribute,
                                          &pressed_attribute};
    float                yOffset       = (bounds.h - H) * 0.5f;

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

    /// 三层渲染完成，根据 Anchor 将 textLayer 合成到最终纹理上
    /// textLayer 宽度由文本内容决定，最终纹理宽度由控件逻辑宽度决定，
    /// 通过 Anchor 确定 textLayer 在最终纹理帧中的水平对齐位置。

    // 根据 Anchor 计算 textLayer 在帧内的水平偏移
    float offsetX = 0.0f;
    switch (VState->Anchor)
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