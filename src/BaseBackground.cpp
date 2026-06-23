
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
        m_cacheTexture.reset();
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
    SDL_Rect bounds = getLogicalBounds();
    m_cacheTexture =
        std::make_shared<Texture>(bounds.w, bounds.h, size_t(1), size_t(1));
    generateTexture(m_cacheTexture->get());
}

void BaseBackground::onExit() { m_cacheTexture.reset(); }

void BaseBackground::Draw()
{
    if (!m_cacheTexture || !m_cacheTexture->get())
        return;

    Rect  logiRect = getLogicalBounds();
    auto &GFX      = OpenCoreManagers::GFXManager.getInstance();
    Rect  VRect    = GFX.getSccissorRect();

    if (VState->getAlpha() > 0.0f && visible(logiRect, VRect))
    {
        Rect dstRect = getPhysicalBounds();
        m_cacheTexture->Draw(nullptr, &dstRect, 0.0, nullptr,
                             static_cast<uint8_t>(VState->getAlpha()));
    }
}

void BaseBackground::setNativeScale(uint8_t scale) { nativeScale = scale; }

void BaseBackground::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
}

bool BaseBackground::generateTexture(SDL_Texture *target)
{
    if (!target || !m_cacheTexture)
        return false;

    if (!texture || !texture->get())
    {
        LOG("BaseBackground::generateTexture() 纹理尚未加载");
        return false;
    }

    float targetW, targetH;
    SDL_GetTextureSize(target, &targetW, &targetH);

    Rect srcRect{};
    Rect dstRect{};

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

            texture->Draw(m_cacheTexture.get(), &srcRect, &dstRect, 0.0,
                          nullptr);
        }
    }

    status = BaseBackgroundStatus::ready;
    return true;
}
