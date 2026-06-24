#include "Runtime/Graphics/UI/Symbol.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include "Runtime/Graphics/IDrawableObject/IDrawableObject.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include <SDL3/SDL_render.h>

Symbol::Symbol(std::string_view id, short layer, std::string_view textureName)
    : UIElement(id.data(), layer, nullptr)
{
    auto *pkg = OpenEngine::getInstance().getPackageManager();

    auto tex = pkg->getTextureObject(textureName);
    if (!tex)
    {
        LOG("创建元素 {} 时发生错误，根据指定纹理名称 {} 未找到对应的贴图",
            id.data(), textureName);
        return;
    }

    this->AnimeManager = std::make_unique<AnimationManager>();
    this->VState       = std::make_unique<VisualState>();

    texture = tex;
}

void Symbol::parseEvents(Event *event, float totalTime) {}

void Symbol::onUpdate(float totalTime) { UIElement::onUpdate(totalTime); }

void Symbol::Draw()
{
    UIElement::Draw();

    if (!texture)
        return;

    Rect dstRect = getLogicalBounds();
    Rect VRect   = OpenCoreManagers::GFXManager.getInstance().getSccissorRect();
    if (VState->getAlpha() <= 0.0f || !visible(dstRect, VRect))
        return;

    Rect srcRect = texture->getSubRect(static_cast<int>(m_symbolType));

    texture->Draw(&srcRect, &dstRect, VState->angle, nullptr,
                  static_cast<uint8_t>(VState->getAlpha()));
}
