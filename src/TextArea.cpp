#include "Core/Helpers/Debugger.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
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

void TextArea::onUpdate(float totalTime)
{
    UIElement::onUpdate(totalTime);

    if (!isAnimeFinished())
    {
        refreshTextureCache();
    }
}

void TextArea::Draw()
{
    if (m_textureDirty)
    {
        if (!m_textContent.empty())
            refreshTextureCache();
        m_textureDirty = false;
    }
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

void TextArea::refreshTextureCache()
{
    Rect  loRect = getLogicalBounds();
    auto &GFX    = GraphicsManager::getInstance();

    m_textureCache = std::make_shared<Texture>(static_cast<uint16_t>(loRect.w),
                                               static_cast<uint16_t>(loRect.h),
                                               size_t(1), size_t(1));
    if (!m_textureCache || !m_textureCache->get())
        return;

    GFX.setRenderTarget(m_textureCache->get());

    auto font = OpenEngine::getInstance().getPackageManager()->getFont(
        fontName, m_fontSize);
    if (!font)
    {
        LOG("资源管理器查询字体对象的结果为空");
        GFX.setRenderTarget(nullptr);
        return;
    }

    uint8_t textAlpha = VState->getAlpha();

    SDL_Surface *text = TTF_RenderText_Blended(
        font.get(), m_textContent.c_str(), m_textContent.length(),
        {m_colorR, m_colorG, m_colorB, textAlpha});
    SDL_Texture *rawBuffer =
        SDL_CreateTextureFromSurface(GFX.getRenderer(), text);
    SDL_DestroySurface(text);

    if (!rawBuffer)
    {
        LOG("Failed to generate text texture");
        GFX.setRenderTarget(nullptr);
        return;
    }

    auto textTex = std::make_shared<Texture>(
        1, 1, std::shared_ptr<SDL_Texture>(rawBuffer, SDL_DestroyTexture));

    float texW = static_cast<float>(textTex->width);
    float texH = static_cast<float>(textTex->height);

    Rect dstRect = {0, 0, m_fontSize * (texW / texH), m_fontSize * 1.0f};

    if (m_aligncenter)
    {
        dstRect.x = (loRect.w - dstRect.w) * 0.5f;
        dstRect.y = (loRect.h - dstRect.h) * 0.5f;
    }

    if (m_shadowEnable)
    {
        uint8_t      shadowAlpha   = VState->getAlpha() * transparency;
        SDL_Surface *shadowSurface = TTF_RenderText_Blended(
            font.get(), m_textContent.c_str(), m_textContent.length(),
            {0, 0, 0, shadowAlpha});

        SDL_Texture *rawShadow =
            SDL_CreateTextureFromSurface(GFX.getRenderer(), shadowSurface);
        SDL_DestroySurface(shadowSurface);

        if (rawShadow)
        {
            auto shadowTex = std::make_shared<Texture>(
                1, 1,
                std::shared_ptr<SDL_Texture>(rawShadow, SDL_DestroyTexture));

            Rect shadowRect = dstRect;
            shadowRect.x += m_shadowOffset;
            shadowRect.y += m_shadowOffset;

            shadowTex->Draw(nullptr, &shadowRect, 0.0f, nullptr);
        }
    }

    textTex->Draw(nullptr, &dstRect, 0.0f, nullptr);

    GFX.setRenderTarget(nullptr);
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
