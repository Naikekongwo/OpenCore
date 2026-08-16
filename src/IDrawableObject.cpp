
#include "Runtime/Graphics/IDrawableObject/IDrawableObject.hpp"
#include "OpenCore.hpp"
#include <cstdint>
#include <optional>

void IDrawableObject::setSequential(bool sequential)
{
    AnimeManager->setSequence(sequential);
}

void IDrawableObject::onUpdate(float totalTime)
{
    AnimeManager->onUpdate(totalTime, *VState);
}

void IDrawableObject::parseEvents(Event *event, float totalTime)
{
    // 空置
}

void IDrawableObject::setAnchor(AnchorPoint anchor) { VState->Anchor = anchor; }

void IDrawableObject::setPosition(float x, float y)
{
    positionIsRelative_ = false; // 绝对像素模式
    applyPosition(x, y);
}

void IDrawableObject::setPositeRelative(float x, float y)
{
    positionIsRelative_ = true; // 强制相对模式
    applyPosition(x, y);
}

void IDrawableObject::applyPosition(float x, float y)
{
    SDL_Rect parentRect{
        0, 0,
        OpenEngine::getInstance().getGameInfo()->_graphicsInfo.resolutionWidth,
        OpenEngine::getInstance()
            .getGameInfo()
            ->_graphicsInfo.resolutionHeight};

    if (parentContainer != nullptr)
    {
        parentRect = parentContainer->getLogicalBounds();
    }

    float absX, absY;
    if (positionIsRelative_)
    {
        // 强制相对（PositeR）：x/y 直接作为父容器宽高的倍数（可 >1.0 表示超出边界）
        absX = parentRect.x + x * parentRect.w;
        absY = parentRect.y + y * parentRect.h;
    }
    else
    {
        // 绝对像素（Posite）：x/y 直接作为像素坐标，不做任何自动判断
        absX = x;
        absY = y;
    }

    VState->Position[0] = absX;
    VState->Position[1] = absY;
}

SDL_Rect IDrawableObject::getPhysicalBounds()
{
    return (magnetFactor == 0) ? getLogicalBounds()
                               : magnetRect(getLogicalBounds());
}

SDL_Rect IDrawableObject::getLogicalBounds()
{
    if (!VState)
    {
        LOG("获取逻辑骨架失败，因为VisualState尚未初始化");
        return SDL_Rect{0, 0, 0, 0};
    }

    const auto &state = *VState;

    float logicalWidth  = absWidth * state.scale[0];
    float logicalHeight = absHeight * state.scale[1];

    float logicalX = state.Position[0];
    float logicalY = state.Position[1];

    switch (state.Anchor)
    {
    case AnchorPoint::TopLeft:
        break;
    case AnchorPoint::TopCenter:
        logicalX -= logicalWidth * 0.5f;
        break;
    case AnchorPoint::TopRight:
        logicalX -= logicalWidth;
        break;
    case AnchorPoint::MiddleLeft:
        logicalY -= logicalHeight * 0.5f;
        break;
    case AnchorPoint::Center:
        logicalX -= logicalWidth * 0.5f;
        logicalY -= logicalHeight * 0.5f;
        break;
    case AnchorPoint::MiddleRight:
        logicalX -= logicalWidth;
        logicalY -= logicalHeight * 0.5f;
        break;
    case AnchorPoint::BottomLeft:
        logicalY -= logicalHeight;
        break;
    case AnchorPoint::BottomCenter:
        logicalX -= logicalWidth * 0.5f;
        logicalY -= logicalHeight;
        break;
    case AnchorPoint::BottomRight:
        logicalX -= logicalWidth;
        logicalY -= logicalHeight;
        break;
    }

    return SDL_Rect{static_cast<int>(std::round(logicalX)),
                    static_cast<int>(std::round(logicalY)),
                    static_cast<int>(std::round(logicalWidth)),
                    static_cast<int>(std::round(logicalHeight))};
}

void IDrawableObject::setScale(float w, float h)
{
    scaleIsRelative_ = false; // 绝对像素模式
    applyScale(w, h);
}

void IDrawableObject::setScaleRelative(float w, float h)
{
    scaleIsRelative_ = true; // 强制相对模式
    applyScale(w, h);
}

void IDrawableObject::applyScale(float w, float h)
{
    SDL_Rect parentRect{
        0, 0,
        OpenEngine::getInstance().getGameInfo()->_graphicsInfo.resolutionWidth,
        OpenEngine::getInstance()
            .getGameInfo()
            ->_graphicsInfo.resolutionHeight};

    if (parentContainer)
    {
        parentRect = parentContainer->getLogicalBounds();
    }

    // 保存原始参数，供 changeTexture 重算使用
    scaleArgs_[0] = w;
    scaleArgs_[1] = h;

    float wph = 1.0f;
    if (scaleIsRelative_)
    {
        // 相对模式（ScaleR）：w/h 是父容器宽高的倍数
        if (w * h == 0.0f)
        {
            // 固定宽高比模式 —— 纹理必须持有 SDL_Texture
            wph = texture->getTextureRatio();
            absWidth =
                (w == 0.0f) ? h * parentRect.h * wph : parentRect.w * w;
            absHeight =
                (h == 0.0f) ? (w * parentRect.w) / wph : parentRect.h * h;
            LOG("元素为固定宽高比 ID:{}, {}, {}, {}", id.c_str(), absWidth,
                absHeight, wph);
        }
        else
        {
            absWidth  = w * parentRect.w;
            absHeight = h * parentRect.h;
        }
    }
    else
    {
        // 绝对像素模式（Scale）：w/h 直接作为宽高，0 表示按纹理宽高比
        if (w * h == 0.0f)
        {
            wph = texture->getTextureRatio();
            absWidth  = (w == 0.0f) ? h * wph : w;
            absHeight = (h == 0.0f) ? w / wph : h;
            LOG("元素为固定宽高比 ID:{}, {}, {}, {}", id.c_str(), absWidth,
                absHeight, wph);
        }
        else
        {
            absWidth  = w;
            absHeight = h;
        }
    }
}

AnimationPipeline IDrawableObject::Animate()
{
    // return AnimationPipeline(AnimeManager.get());
    // 上面是系统管线
    return AnimationPipeline(AnimeManager.get(), nullptr);
}

DrawableConfigurator IDrawableObject::Configure()
{
    return DrawableConfigurator(this);
}

bool IDrawableObject::isAnimeFinished() const
{
    return AnimeManager->isFinished();
}

void IDrawableObject::changeTexture(shared_ptr<Texture> newTexture)
{
    if (newTexture)
    {
        texture = newTexture;
        // 按之前决定的相对坐标（如 w=1.0, h=0 → 横向铺满）配合新纹理尺寸重算，
        // 保留原缩放模式（Scale 自动 / ScaleR 强制相对）
        applyScale(scaleArgs_[0], scaleArgs_[1]);
    }
    else
    {
        LOG("传入的新纹理非法");
    }
}

IDrawableObject::IDrawableObject()
{
    this->id    = "null";
    this->layer = 0;

    AnimeManager = std::make_unique<AnimationManager>();
    VState       = std::make_unique<VisualState>();

    // 此构造器理应给那些不需要纹理的元素使用，所以不加载纹理
}

IDrawableObject::IDrawableObject(string_view id, short layer,
                                 string_view textureName)
{
    this->id    = id;
    this->layer = layer;

    AnimeManager = std::make_unique<AnimationManager>();
    VState       = std::make_unique<VisualState>();

    auto tex = OpenEngine::getInstance().getPackageManager()->getTextureObject(
        textureName);
    if (tex)
    {
        texture = tex;
    }
    else
    {
        LOG("元素初始化时候遇到了空纹理，元素ID {} 纹理ID {}", id.data(),
            textureName);
    }
}

void IDrawableObject::setParentContainer(IDrawableObject *parentContainer)
{
    if (parentContainer != nullptr)
    {

        absolutePosite        = false;
        this->parentContainer = parentContainer;

        this->layer = parentContainer->getLayer() + 1;
    }
    else
    {
        absolutePosite = true;
    }
}

void IDrawableObject::setTransparency(float alpha)
{
    VState->transparency = alpha;
}

bool IDrawableObject::onDestroy()
{
    AnimeManager.reset();
    VState.reset();

    texture.reset();

    return true;
}

void IDrawableObject::setMagnetFactor(int Margin)
{
    magnetFactor = Margin;
    this->VState->scale[0] *= 1.05f;
    this->VState->scale[1] *= 1.05f;
}

SDL_Rect IDrawableObject::magnetRect(const SDL_Rect &srcRect) const
{

    if (magnetFactor == 0)
        return srcRect;

    auto &GFX = GraphicsManager::getInstance();

    int winW, winH;
    SDL_GetWindowSize(GFX.getWindow(), &winW, &winH);

    int centerX = winW / 2;
    int centerY = winH / 2;

    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    float normX = (mouseX - centerX) / static_cast<float>(centerX);
    float normY = (mouseY - centerY) / static_cast<float>(centerY);

    normX = std::clamp(normX, -1.0f, 1.0f);
    normY = std::clamp(normY, -1.0f, 1.0f);

    float offsetX = normX * magnetFactor;
    float offsetY = normY * magnetFactor;

    SDL_Rect result = srcRect;
    result.x += static_cast<int>(offsetX);
    result.y += static_cast<int>(offsetY);

    return result;
}
