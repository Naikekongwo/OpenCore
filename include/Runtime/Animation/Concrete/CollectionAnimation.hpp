#pragma once

#include "Runtime/Animation/IAnimation.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include <memory>

using std::unique_ptr;

class AnimationPipeline;

class CollectionAnimation : public IAnimation
{
  public:
    // 构造函数
    CollectionAnimation(AnimationPipeline *parentPipeline,
                        bool               isParalle = true);
    // 需在 cpp 中析构（m_subPipeline 持有不完整类型 AnimationPipeline）
    ~CollectionAnimation() override;

    void onUpdate(float totalTime, VisualState &state) override; // 更新动画
    bool isFinished() const override;                            // 是否结束
    void reset(float totalTime, VisualState &state) override;    // 重置
    bool isLoop() override { return isLooping; }

    AnimationPipeline *Begin();

  private:
    //   Parallel : true 代表动画组顺序执行
    bool                         Parallel  = true;
    bool                         isLooping = false;
    bool                         finished  = false;
    unique_ptr<AnimationManager> AnimeManager;
    AnimationPipeline           *parentPipe = nullptr;
    // 持有 Begin() 创建的子 pipeline，随本对象析构自动释放（修复裸 new 泄漏）
    unique_ptr<AnimationPipeline> m_subPipeline;
};