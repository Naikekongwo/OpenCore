
#include "Runtime/Graphics/UI/ImageBoard.hpp"
#include "OpenCore.hpp"
#include "Runtime/Animation/AnimationPipeline.hpp"
#include "Runtime/Graphics/IDrawableObject/UIElement.hpp"

ImageBoard::ImageBoard(const std::string &id, uint8_t layer,
                       shared_ptr<Texture> texture)
    : UIElement(id, layer, texture)
{
    // id、layer、texture、AnimeManager、VState 均已在基类链中正确初始化
}

void ImageBoard::Draw()
{
    UIElement::Draw();
    Rect dstRect = getLogicalBounds();
    if (!texture || !texture->get())
        return;

    Rect VRect = OpenCoreManagers::GFXManager.getInstance().getSccissorRect();
    ///< 在此处不需要进行PhysicalBounds的判断，如果其逻辑位置在屏幕之外，那就没有必要渲染。
    if (VState->getAlpha() > 0.0f && visible(dstRect, VRect))
    {
        Rect srcRect = texture->getSubRect(VState->getFrameIndex());

        dstRect = magnetRect(dstRect);

        texture->Draw(&srcRect, &dstRect, VState->getAngle(), nullptr,
                      static_cast<uint8_t>(VState->getAlpha()));
    }
}