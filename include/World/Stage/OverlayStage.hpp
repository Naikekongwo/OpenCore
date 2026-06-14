#ifndef _OVERLAY_STAGE_H_
#define _OVERLAY_STAGE_H_

// OverlayStage.hpp
// 覆盖层场景的基类

#include "Stage.hpp"

class OverlayStage : public Stage
{
  public:
    OverlayStage() = default;
    OverlayStage(Timer *timer, StageManager *sController,
                 StageType type = overlayStage)
        : Stage(timer, sController, type)
    {
    }
};

#endif //_OVERLAY_STAGE_H_