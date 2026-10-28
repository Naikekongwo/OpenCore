#include "Runtime/Graphics/UI/ImageBoard.hpp"

#include "OpenCore.hpp"
#include "Runtime/Animation/AnimationPipeline.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"
#include "Runtime/Graphics/Manager/GraphicsManager.hpp"
#include "Runtime/Graphics/UI/RoundedRect.hpp"

#include <memory>

namespace OpenUI
{

ImageBoard::ImageBoard(std::string_view id, uint8_t layer,
                       shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
    // id、layer、texture、AnimeManager、VState 均已在基类链中正确初始化
}

void ImageBoard::onUpdate(float totalTime)
{
    // ImageBoard 直接画外部纹理，不需要基类的离屏内容纹理
    m_textureDirty = false;
    UIElement::onUpdate(totalTime);
}

void ImageBoard::Draw()
{
    UIElement::Draw();

    if (!texture || !texture->get())
        return;

    Rect dstRect = getLogicalBounds();
    if (VState->getAlpha() <= 0.0f ||
        !visible(dstRect, OpenCoreManagers::GFXManager.getSccissorRect()))
        return;

    const float radiusPx = cornerRadiusPixels();
    const float borderPx = hasVisibleBorder() ? borderWidthPixels() : 0.0f;

    dstRect = magnetRect(dstRect);

    if (radiusPx <= 0.0f)
    {
        Rect imgDst = dstRect;
        imgDst.x += borderPx;
        imgDst.y += borderPx;
        imgDst.w -= borderPx * 2.0f;
        imgDst.h -= borderPx * 2.0f;
        if (imgDst.w <= 0.0f || imgDst.h <= 0.0f)
            return;

        Rect srcRect = texture->getSubRect(VState->getFrameIndex());
        texture->Draw(&srcRect, &imgDst, VState->getAngle(), nullptr,
                      static_cast<uint8_t>(VState->getAlpha()));
        return;
    }

    ensureMaskedTexture(getLogicalBounds(), radiusPx, borderPx);
    if (!m_compositeCache || !m_compositeCache->get())
        return;

    m_compositeCache->Draw(nullptr, &dstRect, VState->getAngle(), nullptr,
                           static_cast<uint8_t>(VState->getAlpha()));
}

bool ImageBoard::onDestroy()
{
    m_maskCache.reset();
    m_compositeCache.reset();
    return UIElement::onDestroy();
}

void ImageBoard::ensureMaskedTexture(const SDL_Rect &bounds, float radiusPx,
                                     float borderPx)
{
    const size_t frame = VState->getFrameIndex();
    const bool   contentMatches =
        m_compositeCache && m_maskedSource == texture.get() &&
        m_maskedFrame == frame && m_maskedRadius == radiusPx &&
        m_maskedBorder == borderPx;
    const bool sizeMatches = m_compositeCache && m_maskedWidth == bounds.w &&
                             m_maskedHeight == bounds.h;

    if (contentMatches && sizeMatches)
        return;

    // 缩放动画期间尺寸每帧都变，先复用旧缓存，动画结束再按最终尺寸重建
    if (contentMatches && !sizeMatches && !isAnimeFinished())
        return;

    auto *renderer = GraphicsManager::getInstance().getRenderer();
    if (renderer == nullptr || bounds.w <= 0 || bounds.h <= 0)
        return;

    if (!m_maskCache || m_maskWidth != bounds.w || m_maskHeight != bounds.h ||
        m_maskRadius != radiusPx || m_maskBorder != borderPx)
    {
        m_maskCache  = RoundedRect::BuildMask(bounds, radiusPx, borderPx);
        m_maskWidth  = bounds.w;
        m_maskHeight = bounds.h;
        m_maskRadius = radiusPx;
        m_maskBorder = borderPx;
    }
    if (!m_maskCache || !m_maskCache->get())
        return;

    auto composite = std::make_shared<Texture>(
        static_cast<uint16_t>(bounds.w), static_cast<uint16_t>(bounds.h),
        size_t(1), size_t(1));
    if (!composite || !composite->get())
        return;

    SDL_Texture *previous = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, composite->get());
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    Rect srcRect = texture->getSubRect(frame);
    Rect imgDst{borderPx, borderPx,
                static_cast<float>(bounds.w) - borderPx * 2.0f,
                static_cast<float>(bounds.h) - borderPx * 2.0f};
    if (imgDst.w > 0.0f && imgDst.h > 0.0f)
        texture->Draw(&srcRect, &imgDst, 0.0, nullptr);

    // 只乘蒙版的 alpha，RGB 保持原样：dstA = srcA * dstA
    const SDL_BlendMode maskBlend = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ZERO, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_DST_ALPHA, SDL_BLENDFACTOR_ZERO,
        SDL_BLENDOPERATION_ADD);
    if (maskBlend != SDL_BLENDMODE_INVALID &&
        SDL_SetTextureBlendMode(m_maskCache->get(), maskBlend))
    {
        Rect maskRect{0.0f, 0.0f, static_cast<float>(bounds.w),
                      static_cast<float>(bounds.h)};
        m_maskCache->Draw(nullptr, &maskRect, 0.0, nullptr);
    }

    SDL_SetRenderTarget(renderer, previous);

    m_compositeCache = composite;
    m_maskedSource   = texture.get();
    m_maskedFrame    = frame;
    m_maskedWidth    = bounds.w;
    m_maskedHeight   = bounds.h;
    m_maskedRadius   = radiusPx;
    m_maskedBorder   = borderPx;
}

} // namespace OpenUI
