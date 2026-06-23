#include "Runtime/Graphics/IDrawableObject/Texture.hpp"
#include "OpenCore.hpp"
#include <memory>

Texture::Texture(size_t x, size_t y, shared_ptr<SDL_Texture> tex)
    : xCount(x), yCount(y), texture(std::move(tex))
{
    if (!texture)
    {
        LOG("Texture::Texture() 错误：SDL_Texture 为空，使用后备尺寸");
        width  = 1;
        height = 1;
        return;
    }

    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
    float W_f, H_f;
    SDL_GetTextureSize(texture.get(), &W_f, &H_f);

    width  = static_cast<uint16_t>(W_f);
    height = static_cast<uint16_t>(H_f);

    if (xCount == 0 || yCount == 0)
    {
        LOG("Texture::Texture() 网格参数非法，使用默认值 1x1");
        xCount = 1;
        yCount = 1;
    }

    width /= xCount;
    height /= yCount;
}

Texture::Texture(uint16_t frameW, uint16_t frameH, size_t x, size_t y)
    : xCount(x), yCount(y), width(frameW), height(frameH)
{
    if (xCount == 0)
        xCount = 1;
    if (yCount == 0)
        yCount = 1;

    uint16_t totalW = width * static_cast<uint16_t>(xCount);
    uint16_t totalH = height * static_cast<uint16_t>(yCount);

    texture =
        GraphicsManager::getInstance().createTextureShared(totalW, totalH);

    if (!texture)
    {
        LOG("Texture::Texture(off-screen) 创建失败 ({}x{})", totalW, totalH);
        return;
    }

    SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);
}

SDL_Rect Texture::getSubRect(size_t index)
{
    // 构造一个0矩阵的常量，避免重复生成
    static const SDL_Rect emptyRect{0, 0, 0, 0};
    if (index < 0 || index >= Size())
    {
        LOG("Texture::getSubRect() index out of range: {}", index);
        return emptyRect;
    }

    if (Size() == 1)
    {
        // 一张大图
        return SDL_Rect{0, 0, width, height};
    }
    else
    {
        uint8_t col = index % xCount;
        uint8_t row = index / xCount;

        return SDL_Rect{col * width, row * height, width, height};
    }
}

int Texture::Draw(const Rect *srcRect, const Rect *dstRect, double angle,
                  const Point *center, uint8_t alpha)
{
    auto &gfx      = GraphicsManager::getInstance();
    auto *renderer = gfx.getRenderer();

    SDL_SetTextureAlphaMod(get(), alpha);

    SDL_FRect  srcF, dstF;
    SDL_FPoint centerF;
    if (srcRect)
        srcF = *srcRect;
    if (dstRect)
        dstF = *dstRect;
    if (center)
        centerF = *center;

    int result = SDL_RenderTextureRotated(
        renderer, get(), srcRect ? &srcF : nullptr, dstRect ? &dstF : nullptr,
        angle, center ? &centerF : nullptr, SDL_FLIP_NONE);

    SDL_SetTextureAlphaMod(get(), 255);
    return result;
}

int Texture::Draw(Texture *target, const Rect *srcRect, const Rect *dstRect,
                  double angle, const Point *center, uint8_t alpha)
{
    auto &gfx      = GraphicsManager::getInstance();
    auto *renderer = gfx.getRenderer();
    auto *prev     = SDL_GetRenderTarget(renderer);

    SDL_SetRenderTarget(renderer, target->get());
    int result = Draw(srcRect, dstRect, angle, center, alpha);
    SDL_SetRenderTarget(renderer, prev);

    return result;
}

SDL_Texture *Texture::get() const { return texture.get(); }

uint16_t Texture::Size() const noexcept { return xCount * yCount; }

void Texture::QueryTexture(int &w, int &h) const
{
    w = width;
    h = height;
}

void Texture::QueryRawTexture(int &w, int &h) const
{
    w = width * static_cast<int>(xCount);
    h = height * static_cast<int>(yCount);
}

Texture::operator SDL_Texture *() { return texture.get(); }

float Texture::getTextureRatio() const noexcept
{
    return static_cast<float>(width) / static_cast<float>(height);
}