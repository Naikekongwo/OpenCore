
#include "OpenCore.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstddef>
#include <memory>

BaseBackground::BaseBackground(std::string_view id, uint8_t layer,
                               shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
    // UIElement 构造器已处理 id、layer、texture 的初始化
}

void BaseBackground::onUpdate(float totalTime)
{
    UIElement::onUpdate(totalTime);
}

void BaseBackground::Draw()
{
    UIElement::Draw();

    if (!m_textureCache || !m_textureCache->get())
        return;

    Rect  logiRect = getLogicalBounds();
    auto &GFX      = OpenCoreManagers::GFXManager.getInstance();
    Rect  VRect    = GFX.getSccissorRect();

    if (VState->getAlpha() > 0.0f && visible(logiRect, VRect))
    {
        Rect dstRect = getPhysicalBounds();
        m_textureCache->Draw(nullptr, &dstRect, 0.0, nullptr,
                             static_cast<uint8_t>(VState->getAlpha()));
    }
}

void BaseBackground::setNativeScale(uint8_t scale) { nativeScale = scale; }

void BaseBackground::parseEvents(Event *event, float totalTime)
{
    UIElement::parseEvents(event, totalTime);
}

bool BaseBackground::generateTexture()
{
    if (!m_textureCache || !m_textureCache->get())
        return false;

    auto *raw = m_textureCache->get();

    if (!texture || !texture->get())
    {
        LOG("纹理尚未加载");
        return false;
    }

    float targetW, targetH;
    SDL_GetTextureSize(raw, &targetW, &targetH);

    // 源纹理强制按 3x3 划分，不依赖纹理自身登记的网格
    // （同一纹理可能两用：普通显示时按登记网格，作 BaseBackground 时必须 3x3）
    float srcW, srcH;
    SDL_GetTextureSize(texture->get(), &srcW, &srcH);
    srcW /= 3.0f;
    srcH /= 3.0f;

    Rect srcRect{};
    Rect dstRect{};

    for (int row = 0; row < 3; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            srcRect = Rect{col * srcW, row * srcH, srcW, srcH};

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

            texture->Draw(m_textureCache.get(), &srcRect, &dstRect, 0.0,
                          nullptr);
        }
    }

    return true;
}
