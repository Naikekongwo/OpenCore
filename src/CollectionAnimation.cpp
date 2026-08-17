
#include "OpenCore.hpp"
#include <memory>

CollectionAnimation::CollectionAnimation(AnimationPipeline *parentPipeline,
                                         bool               isParalle)
{
    AnimeManager     = std::make_unique<AnimationManager>();
    this->parentPipe = parentPipeline;

    Parallel = isParalle;
}

CollectionAnimation::~CollectionAnimation() = default;

AnimationPipeline *CollectionAnimation::Begin()
{
    m_subPipeline =
        std::make_unique<AnimationPipeline>(AnimeManager.get(), parentPipe);
    return m_subPipeline.get();
}

void CollectionAnimation::onUpdate(float totalTime, VisualState &state)
{
    AnimeManager->onUpdate(totalTime, state);
}

bool CollectionAnimation::isFinished() const
{
    return AnimeManager->isFinished();
}

void CollectionAnimation::reset(float totalTime, VisualState &state)
{
    AnimeManager->reset(totalTime, state);
}