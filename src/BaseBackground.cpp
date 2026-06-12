
#include "OpenCore.hpp"
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstddef>
#include <memory>

BaseBackground::BaseBackground(const std::string &id, uint8_t layer,
                               unique_ptr<Texture> texture)
    : UIElement(id, layer, std::move(texture))
{
    // id、layer、texture 均已在 UIElement 初始化列表中正确设置
    this->id = id;
    this->layer = layer;

    if (!texture)
    {
        LOG("纹理非法！");
        return;
    }

    this->texture = std::move(texture);
}

BaseBackground::~BaseBackground()
{
    if (TextureCache)
        SDL_DestroyTexture(TextureCache);
}

bool BaseBackground::onDestroy()
{
    if (TextureCache)
    {
        SDL_DestroyTexture(TextureCache);
        TextureCache = nullptr;
    }
    return UIElement::onDestroy();
}

void BaseBackground::onUpdate(float totalTime)
{
    if (status != BaseBackgroundStatus::ready)
    {
        onEnter();
    }
}
void BaseBackground::onEnter()
{
    auto &GFX = OpenCoreManagers::GFXManager;
    SDL_Rect bounds = getLogicalBounds();
    TextureCache = GFX.createTexture(bounds.w, bounds.h);
    generateTexture(TextureCache);
}

void BaseBackground::onExit()
{
    if (TextureCache)
    {
        SDL_DestroyTexture(TextureCache);
        TextureCache = nullptr;
    }
}

void BaseBackground::Draw()
{
    auto &GFX = OpenCoreManagers::GFXManager.getInstance();

    Rect logiRect = getLogicalBounds();
    Rect VRect = GFX.getSccissorRect();

    if (TextureCache && visible(logiRect, VRect) && VState->getAlpha() > 0.0f)
    {
        Rect dstRect = getPhysicalBounds();

        SDL_SetTextureAlphaMod(TextureCache, VState->getAlpha());
        GFX.Draw(TextureCache, NULL, &dstRect, 0, 0);
    }
}

void BaseBackground::setNativeScale(uint8_t scale) { nativeScale = scale; }

void BaseBackground::parseEvents(Event *event, float totalTime)
{
    const SDL_Event &sdlEvent = event->GetSDLEvent();
    switch (sdlEvent.type)
    {
    case SDL_EVENT_WINDOW_RESIZED:
    {
        auto &GFX = OpenCoreManagers::GFXManager;
        SDL_Rect bounds = getLogicalBounds();
        if (TextureCache)
        {
            SDL_DestroyTexture(TextureCache);
        }
        TextureCache = GFX.createTexture(bounds.w, bounds.h);
        generateTexture(TextureCache);
    }
    default:
        break;
    }
}

bool BaseBackground::generateTexture(SDL_Texture *target)
{
    auto &GFX = GraphicsManager::getInstance();
    if (!target)
        return false;

    GFX.setRenderTarget(target);

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
