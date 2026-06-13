#include "Asset/ResourceManager.hpp"
#include "Core/Helpers/Debugger.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Graphics/IDrawableObject/Texture.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>

TextArea::TextArea(const string &id, uint8_t layer, short fontID)
    : UIElement(id, layer, nullptr)
{
    this->fontID = fontID;
    m_textureDirty = false; // 初始无文本，无需刷新
    LOG("文本框创建，字体代号:{}", fontID);
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
    auto GFX = OpenCoreManagers::GFXManager.getInstance();
    Rect VRect = GFX.getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
    {
        return;
    }

    // <渲染逻辑>

    GFX.Draw(m_textureCache, nullptr, &dstRect, 0.0f, nullptr);
    // <渲染逻辑>
}

void TextArea::setText(string_view textContent)
{
    // 设置后刷新缓存
    m_textContent = textContent;
    m_textureDirty = true;
}

void TextArea::setFontSize(short fontSize)
{
    // 设置字号后刷新缓存
    m_fontSize = fontSize;
    m_textureDirty = true;
}

void TextArea::refreshTextureCache()
{
    Rect loRect = getLogicalBounds();
    auto &GFX = GraphicsManager::getInstance();

    auto target = GFX.createTexture(loRect.w, loRect.h);

    if (!target)
        return;

    GFX.setRenderTarget(target);

    auto font = OpenCoreManagers::ResManager.GetFont(fontID);
    if (!font)
    {
        LOG("资源管理器查询字体对象的结果为空");
        GFX.setRenderTarget(nullptr);
        SDL_DestroyTexture(target);
        return;
    }

    uint8_t textAlpha = VState->getAlpha();

    SDL_Surface *text = TTF_RenderText_Blended(
        font, m_textContent.c_str(), m_textContent.length(),
        {m_colorR, m_colorG, m_colorB, textAlpha});
    SDL_Texture *textBuffer =
        SDL_CreateTextureFromSurface(GFX.getRenderer(), text);

    SDL_DestroySurface(text);

    if (!textBuffer)
    {
        LOG("Failed to generate text texture");
        GFX.setRenderTarget(nullptr);
        SDL_DestroyTexture(target);
        return;
    }

    float texW, texH;

    SDL_GetTextureSize(textBuffer, &texW, &texH);

    Rect dstRect = {0, 0, m_fontSize * (texW / texH), m_fontSize * 1.0f};

    if (m_aligncenter)
    {
        dstRect.x = (loRect.w - dstRect.w) * 0.5f;
        dstRect.y = (loRect.h - dstRect.h) * 0.5f;
    }

    if (m_shadowEnable)
    {
        uint8_t shadowAlpha = VState->getAlpha() * transparency;
        SDL_Surface *shadowSurface = TTF_RenderText_Blended(
            font, m_textContent.c_str(), m_textContent.length(),
            {0, 0, 0, shadowAlpha});

        SDL_Texture *shadowBuffer =
            SDL_CreateTextureFromSurface(GFX.getRenderer(), shadowSurface);

        Rect shadowRect = dstRect;
        shadowRect.x += m_shadowOffset;
        shadowRect.y += m_shadowOffset;

        GFX.Draw(shadowBuffer, nullptr, &shadowRect, 0.0f, nullptr);

        SDL_DestroySurface(shadowSurface);
        SDL_DestroyTexture(shadowBuffer);
    }

    GFX.Draw(textBuffer, nullptr, &dstRect, 0.0f, nullptr);

    SDL_DestroyTexture(textBuffer);

    GFX.setRenderTarget(nullptr);

    // if (m_textureCache)
    //     SDL_DestroyTexture(m_textureCache);
    m_textureCache = target;
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
