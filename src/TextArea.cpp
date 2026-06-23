#include "Core/Helpers/Debugger.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/IDrawableObject/Texture.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>

TextArea::TextArea(const string &id, uint8_t layer, std::string_view fontName)
    : UIElement(id, layer, nullptr)
{
    this->fontName = fontName;
    m_textureDirty = false; // 初始无文本，无需刷新
    LOG("文本框创建，字体名称:{}", fontName);
}

void TextArea::onUpdate(float totalTime) { UIElement::onUpdate(totalTime); }

void TextArea::Draw()
{
    // 检查是否需要显示
    if (m_textContent.empty())
    {
        return;
    }

    Rect dstRect = getLogicalBounds();
    auto GFX     = OpenCoreManagers::GFXManager.getInstance();
    Rect VRect   = GFX.getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
    {
        return;
    }

    // <渲染逻辑>

    if (m_textureCache)
        m_textureCache->Draw(nullptr, &dstRect, 0.0, nullptr,
                             static_cast<uint8_t>(VState->getAlpha()));
    // <渲染逻辑>
}

void TextArea::setText(string_view textContent)
{
    // 设置后刷新缓存
    m_textContent  = textContent;
    m_textureDirty = true;
}

void TextArea::setFontSize(short fontSize)
{
    // 设置字号后刷新缓存
    m_fontSize     = fontSize;
    m_textureDirty = true;
}

bool TextArea::generateTexture(SDL_Texture *target)
{
    if (!target)
        return false;
    auto &GFX = GraphicsManager::getInstance();

    GFX.setRenderTarget(target);
    GFX.setRenderTarget(nullptr);

    TextAttribute attr;
    attr.fontName = fontName;
    attr.fontSize = m_fontSize;

    int texW = 0, texH = 0;
    Text::Measure(m_textContent, attr, texW, texH);

    Rect loRect = getLogicalBounds();
    Rect dstRect;
    dstRect.h = static_cast<float>(m_fontSize);
    dstRect.w =
        (texH > 0) ? dstRect.h * (static_cast<float>(texW) / texH) : 0.0f;

    if (m_aligncenter)
    {
        dstRect.x = (loRect.w - dstRect.w) * 0.5f;
        dstRect.y = (loRect.h - dstRect.h) * 0.5f;
    }

    float textAlpha = VState->getAlpha() / 255.0f;
    attr.color  = Color(m_colorR / 255.0f, m_colorG / 255.0f, m_colorB / 255.0f,
                        textAlpha);
    attr.option = RENDER_TEXT;
    if (m_shadowEnable)
        attr.option =
            static_cast<TextRenderOption>(attr.option | RENDER_SHADOW);
    attr.option = static_cast<TextRenderOption>(attr.option | RENDER_GRADIENT);
    attr.gradientColor = Color(0, 0, 1.0f, 1.0f);

    auto wrapper = std::make_shared<Texture>(
        1, 1, std::shared_ptr<SDL_Texture>(target, [](SDL_Texture *) {}));

    Text::Draw(wrapper.get(), &dstRect, m_textContent, attr);
    return true;
}

void TextArea::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
    // m_textureDirty 已在基类 parseEvents 中由 WINDOW_RESIZED 设置
}

void TextArea::setShadow(bool enableTag, int shadowOffset)
{
    this->m_shadowEnable = enableTag;

    this->m_shadowOffset =
        (shadowOffset <= 0 | shadowOffset > 255) ? 5 : shadowOffset;

    LOG("调整了阴影选项，当前状态为{}，阴影位移为 {}",
        (enableTag) ? "开" : "关", m_shadowOffset);
}
