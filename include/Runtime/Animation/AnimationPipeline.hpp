#pragma once

// AnimationPipeline.hpp
// 这是一个Pipeline的模板类
// 我们在实际游戏中引用的是一个叫做AnimationPipeline的类，继承自本类

#include "Runtime/Animation/Concrete/CollectionAnimation.hpp"
#include "Runtime/Animation/Concrete/FadeAnimation.hpp"
#include "Runtime/Animation/Concrete/FrameAnimation.hpp"
#include "Runtime/Animation/Concrete/MoveAnimation.hpp"
#include "Runtime/Animation/Concrete/RotateAnimation.hpp"
#include "Runtime/Animation/Concrete/ScaleAnimation.hpp"
#include "Runtime/Animation/Concrete/TimerAnimation.hpp"

#include "Runtime/Animation/Manager/AnimationManager.hpp"

#include <cstdint>
#include <memory>
#include <vector>

using std::shared_ptr;
using std::vector;

class AnimationPipeline
{
  public:
    AnimationPipeline(AnimationManager *AnimeManager,
                      AnimationPipeline *parentPipeline);

    // 添加帧动画
    AnimationPipeline &Frame(uint8_t totalFrames, uint8_t FPS,
                             bool isLooping = false);
    // 添加渐变动画
    AnimationPipeline &Fade(float startAlpha, float endAlpha, float duration,
                            bool isLooping = false);
    // 添加移动动画（绝对像素）
    AnimationPipeline &Move(uint16_t startX, uint16_t startY, uint16_t endX,
                            uint16_t endY, float duration,
                            bool isLooping = false);
    // 添加移动动画（相对百分比 0.0~1.0）
    AnimationPipeline &MoveR(float startX, float startY, float endX, float endY,
                             float duration, bool isLooping = false);
    // 添加缩放动画
    AnimationPipeline &Scale(float startScale, float endScale, float duration,
                             bool isLooping = false);
    // 添加旋转动画
    AnimationPipeline &Rotate(float startAngle, float endAngle, float duration,
                              bool isLooping = false);
    // 添加定时器动画
    AnimationPipeline &Timer(float duration);

    AnimationPipeline *SubStart(bool isParalle);

    AnimationPipeline &SubEnd();

    // 执行，将所有动画加入管理器
    void Commit();

  protected:
    AnimationPipeline *parentPipeline = nullptr;
    AnimationManager *manager = nullptr;       // 动画管理器指针
    vector<shared_ptr<IAnimation>> animations; // 存储动画的容器
};