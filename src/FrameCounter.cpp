
#include "OpenCore.hpp"

FrameCounter::FrameCounter(const std::string &id, uint8_t layer)
    : UIElement(id, layer, nullptr)
{
    this->VState       = std::make_unique<VisualState>();
    this->AnimeManager = std::make_unique<AnimationManager>();
}

void FrameCounter::parseEvents(Event *event, float totalTime) {}

void FrameCounter::Draw() // 这个帧数应该是常态显示的吧，所以未添加可见判断
{
    UIElement::Draw();

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
    Color       color   = White;

    SDL_Surface *textSurface = TTF_RenderText_Blended(
        font.get(), fpsText.c_str(), fpsText.length(), color);
    if (!textSurface)
        return;

    auto        &GFX = GraphicsManager::getInstance();
    SDL_Texture *raw =
        SDL_CreateTextureFromSurface(GFX.getRenderer(), textSurface);
    SDL_DestroySurface(textSurface);
    if (!raw)
        return;

    auto tex = std::make_shared<Texture>(
        1, 1, std::shared_ptr<SDL_Texture>(raw, SDL_DestroyTexture));

    Rect dstRect = {0, 0, 0, 0};
    dstRect.w    = tex->width;
    dstRect.h    = tex->height;
    tex->Draw(nullptr, &dstRect, 0.0, nullptr);
}
void FrameCounter::onUpdate(float totalTime)
{
    if (totalTime > lastTime)
    {
        FPS      = 1.0f / (totalTime - lastTime);
        lastTime = totalTime;
    }

    UIElement::onUpdate(totalTime);
}