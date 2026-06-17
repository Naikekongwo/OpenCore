
#include "OpenCore.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstddef>
#include <memory>

BaseBackground::BaseBackground(const std::string &id, uint8_t layer,
                               shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
    // UIElement 构造器已处理 id、layer、texture 的初始化
}

void BaseBackground::onUpdate(float totalTime)
{
    if (m_textureDirty)
    {
        if (m_textureCache)
            SDL_DestroyTexture(m_textureCache);
        m_textureCache = nullptr;
        onEnter();
        m_textureDirty = false;
    }
    if (status != BaseBackgroundStatus::ready)
    {
        onEnter();
    }
}
void BaseBackground::onEnter()
{
    auto    &GFX    = OpenCoreManagers::GFXManager;
    SDL_Rect bounds = getLogicalBounds();
    m_textureCache  = GFX.createTexture(bounds.w, bounds.h);
    generateTexture(m_textureCache);
}

void BaseBackground::onExit()
{
    if (m_textureCache)
    {
        SDL_DestroyTexture(m_textureCache);
        m_textureCache = nullptr;
    }
}

void BaseBackground::Draw()
{
    if (!m_textureCache)
        return;

    Rect  logiRect = getLogicalBounds();
    auto &GFX      = OpenCoreManagers::GFXManager.getInstance();
    Rect  VRect    = GFX.getSccissorRect();

    if (VState->getAlpha() > 0.0f && visible(logiRect, VRect))
    {
        Rect dstRect = getPhysicalBounds();

        SDL_SetTextureAlphaMod(m_textureCache, VState->getAlpha());
        GFX.Draw(m_textureCache, NULL, &dstRect, 0, 0);
    }
}

void BaseBackground::setNativeScale(uint8_t scale) { nativeScale = scale; }

void BaseBackground::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
}

bool BaseBackground::generateTexture(SDL_Texture *target)
{
    auto &GFX = GraphicsManager::getInstance();
    if (!target)
        return false;

    GFX.setRenderTarget(target);

    if (!texture || !texture->get())
    {
        LOG("BaseBackground::generateTexture() 纹理尚未加载");
        GFX.setRenderTarget(nullptr);
        return false;
    }

    float texW, texH;
    SDL_GetTextureSize(texture->get(), &texW, &texH);

    Rect srcRect{};
    Rect dstRect{};

    float targetW, targetH;
    SDL_GetTextureSize(target, &targetW, &targetH);

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            srcRect = texture->getSubRect(row * 3 + col);

            if (col == 0)
            {
                dstRect.x = 0;
                dstRect.w = nativeScale;
            }
            else if (col == 1)
            {
                dstRect.x = nativeScale;
                dstRect.w = targetW - 2 * nativeScale;
            }
            else
            {
                dstRect.x = targetW - nativeScale;
                dstRect.w = nativeScale;
            }

            if (row == 0)
            {
                dstRect.y = 0;
                dstRect.h = nativeScale;
            }
            else if (row == 1)
            {
                dstRect.y = nativeScale;
                dstRect.h = targetH - 2 * nativeScale;
            }
            else
            {
                dstRect.y = targetH - nativeScale;
                dstRect.h = nativeScale;
            }

            GFX.Draw(texture->get(), &srcRect, &dstRect, 0.0, nullptr);
        }
    }

    GFX.setRenderTarget(nullptr);

    status = BaseBackgroundStatus::ready;
    return true;
}
