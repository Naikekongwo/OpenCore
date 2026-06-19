
#include "OpenCore.hpp"

FrameCounter::FrameCounter(const std::string &id, uint8_t layer)
    : UIElement(id, layer, nullptr)
{
    this->VState = std::make_unique<VisualState>();
    this->AnimeManager = std::make_unique<AnimationManager>();
}

void FrameCounter::parseEvents(Event *event, float totalTime) {}

void FrameCounter::Draw() // 这个帧数应该是常态显示的吧，所以未添加可见判断
{
    // 懒加载字体（首次 Draw 时从 PackageManager 加载）
    if (!font)
    {
        auto *pkg = OpenEngine::getInstance().getPackageManager();
        if (pkg)
            font = pkg->getFont(fontName, fontSize);
        if (!font)
            return;
    }

    std::string fpsText = "FPS: " + std::to_string(static_cast<int>(FPS));
    SDL_Color color = {255, 255, 255, 255};

    SDL_Surface *textSurface = TTF_RenderText_Blended(
        font.get(), fpsText.c_str(), fpsText.length(), color);
    if (!textSurface)
        return;

    auto &GFX = GraphicsManager::getInstance();
    SDL_Texture *textTexture =
        SDL_CreateTextureFromSurface(GFX.getRenderer(), textSurface);
    if (!textTexture)
    {
        SDL_DestroySurface(textSurface);
        return;
    }

    Rect dstRect = {0, 0, 0, 0};
    dstRect.w = textSurface->w;
    dstRect.h = textSurface->h;
    GFX.Draw(textTexture, nullptr, &dstRect, 0.0, nullptr);
    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}
void FrameCounter::onUpdate(float totalTime)
{
    if (totalTime > lastTime)
    {
        FPS = 1.0f / (totalTime - lastTime);
        lastTime = totalTime;
    }

    UIElement::onUpdate(totalTime);
}