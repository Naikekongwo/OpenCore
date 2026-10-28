/**
 * @file RoundedRect.cpp
 * @brief 圆角矩形离屏生成的实现。
 *
 * 用圆角矩形的距离场逐像素算覆盖率，直边与拐角共享同一套公式；
 * 只在背景/蒙版重建时执行一次。
 */
#include "Runtime/Graphics/UI/RoundedRect.hpp"

#include "OpenCore.hpp"
#include "Runtime/Graphics/Manager/GraphicsManager.hpp"

#include <algorithm>
#include <cmath>

namespace OpenUI
{

namespace
{

float Clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

// 点到圆角矩形边界的距离：矩形内为负，外为正
float RoundedDistance(float x, float y, float width, float height, float radius)
{
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float qx = std::abs(x - cx) - (width * 0.5f - radius);
    const float qy = std::abs(y - cy) - (height * 0.5f - radius);
    const float ax = std::max(qx, 0.0f);
    const float ay = std::max(qy, 0.0f);

    return std::sqrt(ax * ax + ay * ay) + std::min(std::max(qx, qy), 0.0f) -
           radius;
}

uint8_t ToByte(float value)
{
    return static_cast<uint8_t>(Clamp01(value) * 255.0f + 0.5f);
}

std::shared_ptr<Texture> BuildRoundedTexture(const SDL_Rect &bounds, Color fill,
                                             Color border, float radius,
                                             float borderWidth, bool maskOnly)
{
    if (bounds.w <= 0 || bounds.h <= 0)
        return nullptr;

    const float width  = static_cast<float>(bounds.w);
    const float height = static_cast<float>(bounds.h);
    const float rad    = std::clamp(radius, 0.0f, std::min(width, height) * 0.5f);
    const float bw =
        std::clamp(borderWidth, 0.0f, std::min(width, height) * 0.5f);

    SDL_Renderer *renderer = GraphicsManager::getInstance().getRenderer();
    if (renderer == nullptr)
        return nullptr;

    SDL_Surface *surface =
        SDL_CreateSurface(bounds.w, bounds.h, SDL_PIXELFORMAT_RGBA8888);
    if (surface == nullptr)
        return nullptr;

    if (!SDL_LockSurface(surface))
    {
        SDL_DestroySurface(surface);
        return nullptr;
    }

    const SDL_PixelFormatDetails *format =
        SDL_GetPixelFormatDetails(surface->format);
    auto *pixels       = static_cast<uint32_t *>(surface->pixels);
    const int rowStride = surface->pitch / static_cast<int>(sizeof(uint32_t));

    for (int y = 0; y < bounds.h; ++y)
    {
        for (int x = 0; x < bounds.w; ++x)
        {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;

            const float outerCov =
                Clamp01(0.5f - RoundedDistance(px, py, width, height, rad));

            float innerCov = outerCov;
            if (bw > 0.0f && width - bw * 2.0f > 0.0f &&
                height - bw * 2.0f > 0.0f)
            {
                const float innerRad = std::max(0.0f, rad - bw);
                const float innerDist =
                    RoundedDistance(px, py, width - bw * 2.0f,
                                    height - bw * 2.0f, innerRad) +
                    bw;
                innerCov = Clamp01(0.5f - innerDist);
            }

            if (maskOnly)
            {
                pixels[y * rowStride + x] = SDL_MapRGBA(
                    format, nullptr, 255, 255, 255, ToByte(innerCov));
                continue;
            }

            const float borderCov = std::max(0.0f, outerCov - innerCov);
            const float fillA     = Clamp01(fill.a) * innerCov;
            const float borderA   = Clamp01(border.a) * borderCov;
            const float outA      = borderA + fillA * (1.0f - borderA);

            if (outA <= 0.0001f)
            {
                pixels[y * rowStride + x] = SDL_MapRGBA(format, nullptr, 0, 0,
                                                       0, 0);
                continue;
            }

            const float r =
                (border.r * borderA + fill.r * fillA * (1.0f - borderA)) / outA;
            const float g =
                (border.g * borderA + fill.g * fillA * (1.0f - borderA)) / outA;
            const float b =
                (border.b * borderA + fill.b * fillA * (1.0f - borderA)) / outA;

            pixels[y * rowStride + x] =
                SDL_MapRGBA(format, nullptr, ToByte(r), ToByte(g), ToByte(b),
                            ToByte(outA));
        }
    }

    SDL_UnlockSurface(surface);

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (texture == nullptr)
        return nullptr;

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return std::make_shared<Texture>(
        1, 1, std::shared_ptr<SDL_Texture>(texture, SDL_DestroyTexture));
}

} // namespace

std::shared_ptr<Texture> RoundedRect::Build(const SDL_Rect &bounds,
                                            const Style    &style)
{
    return BuildRoundedTexture(bounds, style.fill, style.border, style.radius,
                               style.borderWidth, false);
}

std::shared_ptr<Texture> RoundedRect::BuildMask(const SDL_Rect &bounds,
                                                float radius, float borderWidth)
{
    return BuildRoundedTexture(bounds, White, None, radius, borderWidth, true);
}

} // namespace OpenUI
