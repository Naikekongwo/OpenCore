#include "OpenCore.hpp"
#include <SDL3_ttf/SDL_ttf.h>

// ── 内部：surface → 绘制到 target ──
static void blitText(Texture *target, const Rect *dstRect, SDL_Surface *surface,
                     SDL_Renderer *rend)
{
    SDL_Texture *raw = SDL_CreateTextureFromSurface(rend, surface);
    if (!raw)
        return;
    SDL_Texture *prev = SDL_GetRenderTarget(rend);
    SDL_SetRenderTarget(rend, target->get());
    SDL_FRect fDst = *dstRect;
    SDL_RenderTextureRotated(rend, raw, nullptr, &fDst, 0.0, nullptr,
                             SDL_FLIP_NONE);
    SDL_SetRenderTarget(rend, prev);
    SDL_DestroyTexture(raw);
}

// ── 内部：将白色文字蒙版染成指定颜色（格式无关）──
static void colorizeMask(SDL_Surface *mask, const SDL_Color &c)
{
    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(mask->format);
    SDL_LockSurface(mask);
    auto *px = static_cast<uint32_t *>(mask->pixels);
    int   n  = mask->w * mask->h;
    for (int i = 0; i < n; i++)
    {
        uint8_t r, g, b, a;
        SDL_GetRGBA(px[i], fmt, nullptr, &r, &g, &b, &a);
        if (a == 0)
            continue;
        px[i] = SDL_MapRGBA(fmt, nullptr, c.r, c.g, c.b, a);
    }
    SDL_UnlockSurface(mask);
}

// ── 内部：在 mask 副本上填充渐变色，alpha 从顶 0 递增到底 255 ──
static SDL_Surface *makeGradientLayer(SDL_Surface *mask, const SDL_Color &gradC)
{
    SDL_Surface *dup = SDL_DuplicateSurface(mask);
    if (!dup)
        return nullptr;

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(dup->format);
    SDL_LockSurface(dup);
    auto *px = static_cast<uint32_t *>(dup->pixels);
    int   w = dup->w, h = dup->h;

    for (int y = 0; y < h; y++)
    {
        float t = static_cast<float>(y) / static_cast<float>(h > 1 ? h - 1 : 1);
        uint8_t newA = static_cast<uint8_t>(255.0f * t);

        for (int x = 0; x < w; x++)
        {
            uint8_t r, g, b, a;
            SDL_GetRGBA(px[y * w + x], fmt, nullptr, &r, &g, &b, &a);
            if (a == 0)
                continue;
            px[y * w + x] =
                SDL_MapRGBA(fmt, nullptr, gradC.r, gradC.g, gradC.b, newA);
        }
    }
    SDL_UnlockSurface(dup);
    return dup;
}

// ── Measure ──

bool Text::Measure(string_view textContent, const TextAttribute &attr,
                   int &outW, int &outH)
{
    if (textContent.empty() || attr.fontSize == 0)
        return false;
    auto pkg  = OpenEngine::getInstance().getPackageManager();
    auto font = pkg->getFont(attr.fontName, attr.fontSize);
    if (!font)
        return false;
    return TTF_GetStringSize(font.get(), textContent.data(), textContent.size(),
                             &outW, &outH);
}

// ── Draw ──

void Text::Draw(Texture *target, const Rect *dstRect, string_view textContent,
                const TextAttribute &attr)
{
    if (!target || textContent.empty() || textContent == " ")
        return;
    if (attr.fontSize == 0)
        return;

    auto pkg  = OpenEngine::getInstance().getPackageManager();
    auto font = pkg->getFont(attr.fontName, attr.fontSize);
    if (!font)
        return;

    auto &gfx  = GraphicsManager::getInstance();
    auto *rend = gfx.getRenderer();

    // ── 生成白色 alpha 蒙版（所有层共用） ──
    SDL_Color    white = {255, 255, 255, 255};
    SDL_Surface *mask  = TTF_RenderText_Blended(font.get(), textContent.data(),
                                                textContent.size(), white);
    if (!mask)
        return;

    // ── SHADOW ──
    if (attr.option & RENDER_SHADOW)
    {
        SDL_Surface *s = SDL_DuplicateSurface(mask);
        if (s)
        {
            colorizeMask(s, {0, 0, 0, 180});
            Rect r = *dstRect;
            r.x += 2.0f;
            r.y += 2.0f;
            blitText(target, &r, s, rend);
            SDL_DestroySurface(s);
        }
    }

    // ── TEXT ──
    colorizeMask(mask, static_cast<SDL_Color>(attr.color));
    blitText(target, dstRect, mask, rend);

    // ── GRADIENT（覆盖在 text 之上） ──
    if ((attr.option & RENDER_GRADIENT) && attr.gradientColor.a > 0.01f)
    {
        SDL_Surface *grad =
            makeGradientLayer(mask, static_cast<SDL_Color>(attr.gradientColor));
        if (grad)
        {
            blitText(target, dstRect, grad, rend);
            SDL_DestroySurface(grad);
        }
    }

    SDL_DestroySurface(mask);
}