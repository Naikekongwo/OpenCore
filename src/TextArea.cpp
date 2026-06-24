#include "OpenCore.hpp"
#include "Runtime/Graphics/IDrawableObject/Text.hpp"
#include "Runtime/Graphics/IDrawableObject/Texture.hpp"

TextArea::TextArea(const string &id, uint8_t layer, std::string_view fontName)
    : UIElement(id, layer, nullptr)
{
    m_textAttr.fontName = fontName;
    m_textAttr.option   = static_cast<TextRenderOption>(
        RENDER_TEXT | RENDER_SHADOW | RENDER_GRADIENT | RENDER_BORDER |
        RENDER_GLOW);
    m_textAttr.gradientColor = Color(0, 0, 1.0f, 1.0f);
    m_textAttr.glowColor     = White;
    m_textAttr.BorderSize    = 2;
    m_textureDirty           = false;
}

void TextArea::onUpdate(float totalTime) { UIElement::onUpdate(totalTime); }

void TextArea::Draw()
{
    UIElement::Draw();

    if (m_textContent.empty())
        return;

    Rect dstRect = getLogicalBounds();
    Rect VRect   = OpenCoreManagers::GFXManager.getInstance().getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    if (m_textureCache)
        m_textureCache->Draw(nullptr, &dstRect, 0.0, nullptr,
                             static_cast<uint8_t>(VState->getAlpha()));
}

void TextArea::setText(string_view textContent)
{
    m_textContent  = textContent;
    m_textureDirty = true;
}

void TextArea::setFontSize(short fontSize)
{
    m_textAttr.fontSize = fontSize;
    m_textureDirty      = true;
}

void TextArea::setShadow(bool enableTag, int shadowOffset)
{
    m_textAttr.option =
        enableTag
            ? static_cast<TextRenderOption>(m_textAttr.option | RENDER_SHADOW)
            : static_cast<TextRenderOption>(m_textAttr.option & ~RENDER_SHADOW);

    m_textureDirty = true;
}

bool TextArea::generateTexture()
{
    if (!m_textureCache || !m_textureCache->get())
        return false;

    auto *raw = m_textureCache->get();

    GraphicsManager::getInstance().setRenderTarget(raw);
    GraphicsManager::getInstance().setRenderTarget(nullptr);

    // 同步 VState 透明度到 attr
    m_textAttr.color.a = VState->getAlpha() / 255.0f;

    int texW = 0, texH = 0;
    Text::Measure(m_textContent, m_textAttr, texW, texH);

    Rect loRect = getLogicalBounds();
    Rect dstRect;
    dstRect.h = static_cast<float>(m_textAttr.fontSize);
    dstRect.w =
        (texH > 0) ? dstRect.h * (static_cast<float>(texW) / texH) : 0.0f;

    if (m_aligncenter)
    {
        dstRect.x = (loRect.w - dstRect.w) * 0.5f;
        dstRect.y = (loRect.h - dstRect.h) * 0.5f;
    }

    auto wrapper = std::make_shared<Texture>(
        1, 1, std::shared_ptr<SDL_Texture>(raw, [](SDL_Texture *) {}));

    Text::Draw(wrapper.get(), &dstRect, m_textContent, m_textAttr);
    return true;
}

void TextArea::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
}
