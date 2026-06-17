#include "Runtime/Graphics/UI/Symbol.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include "Runtime/Graphics/IDrawableObject/IDrawableObject.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include <SDL3/SDL_render.h>

Symbol::Symbol(std::string_view id, short layer, short texMetaID)
    : UIElement(id.data(), layer, nullptr)
{
    auto *TMMGR = OpenEngine::getInstance().getTextureMetaManager();

    auto texOpt = TMMGR->getTexture(texMetaID);
    if (texOpt == std::nullopt)
    {
        LOG("创建元素 {} 时发生错误，根据指定META ID {}未找到对应的贴图",
            id.data(), texMetaID);
        return;
    }

    this->AnimeManager = std::make_unique<AnimationManager>();
    this->VState       = std::make_unique<VisualState>();

    texture = texOpt.value();
}

void Symbol::parseEvents(Event *event, float totalTime) {}

void Symbol::onUpdate(float totalTime) { UIElement::onUpdate(totalTime); }

void Symbol::Draw()
{
    if (!texture)
        return;

    Rect  dstRect = getLogicalBounds();
    auto &GFX     = OpenCoreManagers::GFXManager.getInstance();
    Rect  VRect   = GFX.getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    // 应用透明度效果
    SDL_SetTextureAlphaMod(texture->get(), VState->getAlpha());

    Rect srcRect = texture->getSubRect(static_cast<int>(m_symbolType));

    GFX.Draw(texture->get(), &srcRect, &dstRect, VState->angle, nullptr);

    // 撤销透明度效果
    SDL_SetTextureAlphaMod(texture->get(), 255);
}
