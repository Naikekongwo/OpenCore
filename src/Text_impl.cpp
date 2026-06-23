#include "OpenCore.hpp"
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>

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

// ── 内部：对 alpha 做盒模糊，生成柔和发光层 ──
static SDL_Surface *makeGlowLayer(SDL_Surface *mask, const SDL_Color &glowC,
                                  int radius)
{
    SDL_Surface *dup = SDL_DuplicateSurface(mask);
    if (!dup)
        return nullptr;

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(dup->format);
    int                           w = dup->w, h = dup->h;
    int                           n = w * h;

    SDL_LockSurface(dup);
    auto *px = static_cast<uint32_t *>(dup->pixels);

    // 1. 读取原始 alpha
    std::vector<float> src(n), blur(n);
    for (int i = 0; i < n; i++)
    {
        uint8_t r, g, b, a;
        SDL_GetRGBA(px[i], fmt, nullptr, &r, &g, &b, &a);
        src[i] = a / 255.0f;
    }

    // 2. 水平方向模糊
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            float sum = 0;
            int   cnt = 0;
            for (int dx = -radius; dx <= radius; dx++)
            {
                int sx = x + dx;
                if (sx >= 0 && sx < w)
                {
                    sum += src[y * w + sx];
                    cnt++;
                }
            }
            blur[y * w + x] = sum / cnt;
        }

    // 3. 垂直方向模糊
    src.swap(blur);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            float sum = 0;
            int   cnt = 0;
            for (int dy = -radius; dy <= radius; dy++)
            {
                int sy = y + dy;
                if (sy >= 0 && sy < h)
                {
                    sum += src[sy * w + x];
                    cnt++;
                }
            }
            blur[y * w + x] = sum / cnt;
        }

    // 4. 写入最终像素
    uint8_t glowA = glowC.a;
    for (int i = 0; i < n; i++)
    {
        uint8_t newA = static_cast<uint8_t>(blur[i] * glowA);
        if (newA == 0)
            continue;
        px[i] = SDL_MapRGBA(fmt, nullptr, glowC.r, glowC.g, glowC.b, newA);
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

#pragma region 生成Text层蒙版
    // ── 生成白色 alpha 蒙版（所有层共用） ──
    SDL_Color    white = {255, 255, 255, 255};
    SDL_Surface *mask  = TTF_RenderText_Blended(font.get(), textContent.data(),
                                                textContent.size(), white);
    if (!mask)
        return;
#pragma endregion

#pragma region 外发光层 — 最底层（盒模糊产生柔和边缘）
    if ((attr.option & RENDER_GLOW) && attr.glowColor.a > 0.01f)
    {
        SDL_Color    gc   = static_cast<SDL_Color>(attr.glowColor);
        SDL_Surface *glow = makeGlowLayer(mask, gc, 4);
        if (glow)
        {
            blitText(target, dstRect, glow, rend);
            SDL_DestroySurface(glow);
        }
    }
#pragma endregion

#pragma region 阴影层
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
#pragma endregion

#pragma region 描边层
    if ((attr.option & RENDER_BORDER) && attr.BorderSize > 0)
    {
        SDL_Color bc = static_cast<SDL_Color>(attr.borderColor);
        int       bs = attr.BorderSize;
        for (int dy = -bs; dy <= bs; dy += bs)
            for (int dx = -bs; dx <= bs; dx += bs)
            {
                if (dx == 0 && dy == 0)
                    continue;
                SDL_Surface *s = SDL_DuplicateSurface(mask);
                if (!s)
                    continue;
                colorizeMask(s, bc);
                Rect r = *dstRect;
                r.x += static_cast<float>(dx);
                r.y += static_cast<float>(dy);
                blitText(target, &r, s, rend);
                SDL_DestroySurface(s);
            }
    }
#pragma endregion

#pragma region 生成TextSolid层
    colorizeMask(mask, static_cast<SDL_Color>(attr.color));
    blitText(target, dstRect, mask, rend);
#pragma endregion

#pragma region 渐变层 — 最上层
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
#pragma endregion

    SDL_DestroySurface(mask);
}