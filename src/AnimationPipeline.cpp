// AnimationPipeline.cpp
// Pipeline模板类的实现

#include "Runtime/Animation/AnimationPipeline.hpp"
#include "OpenCore.hpp"

AnimationPipeline::AnimationPipeline(AnimationManager *AnimeManager,
                                     AnimationPipeline *parentPipeline)
    : manager(AnimeManager), parentPipeline(parentPipeline) {};

// 添加帧动画
AnimationPipeline &AnimationPipeline::Frame(uint8_t totalFrames, uint8_t FPS,
                                            bool isLooping)
{
    animations.push_back(
        std::make_shared<FrameAnimation>(totalFrames, FPS, isLooping));
    return *this;
}
// 添加渐变动画
AnimationPipeline &AnimationPipeline::Fade(float startAlpha, float endAlpha,
                                           float duration, bool isLooping)
{
    animations.push_back(std::make_shared<FadeAnimation>(startAlpha, endAlpha,
                                                         duration, isLooping));
    return *this;
}
// 添加移动动画（绝对像素）
AnimationPipeline &AnimationPipeline::Move(uint16_t startX, uint16_t startY,
                                           uint16_t endX, uint16_t endY,
                                           float duration, bool isLooping)
{
    animations.push_back(std::make_shared<MoveAnimation>(
        startX, startY, endX, endY, duration, isLooping));
    return *this;
}
// 添加移动动画（相对百分比 0.0~1.0）
AnimationPipeline &AnimationPipeline::MoveR(float startX, float startY,
                                            float endX, float endY,
                                            float duration, bool isLooping)
{
    auto &gfx = OpenEngine::getInstance().getGameInfo()->_graphicsInfo;
    animations.push_back(std::make_shared<MoveAnimation>(
        static_cast<int16_t>(startX * gfx.resolutionWidth),
        static_cast<int16_t>(startY * gfx.resolutionHeight),
        static_cast<int16_t>(endX * gfx.resolutionWidth),
        static_cast<int16_t>(endY * gfx.resolutionHeight), duration,
        isLooping));
    return *this;
}

// 添加缩放动画
AnimationPipeline &AnimationPipeline::Scale(float startScale, float endScale,
                                            float duration, bool isLooping)
{
    animations.push_back(std::make_shared<ScaleAnimation>(startScale, endScale,
                                                          duration, isLooping));
    return *this;
}
// 添加旋转动画
AnimationPipeline &AnimationPipeline::Rotate(float startAngle, float endAngle,
                                             float duration, bool isLooping)
{
    animations.push_back(std::make_shared<RotateAnimation>(
        startAngle, endAngle, duration, isLooping));
    return *this;
}
// 添加定时器动画
AnimationPipeline &AnimationPipeline::Timer(float duration)
{
    animations.push_back(std::make_shared<TimerAnimation>(duration));
    return *this;
}

AnimationPipeline *AnimationPipeline::SubStart(bool isParalle)
{
    auto colani = std::make_shared<CollectionAnimation>(this, isParalle);
    animations.push_back(colani);
    // colani 是集合的共享指针
    return colani->Begin();
}

AnimationPipeline &AnimationPipeline::SubEnd()
{
    Commit();
    // 将当前子段的动画全部返回
    return *parentPipeline;
}

// 执行，将所有动画加入管理器
void AnimationPipeline::Commit()
{
    for (auto &anime : animations)
    {
        manager->pushAnimation(anime);
    }
    animations.clear();
}
