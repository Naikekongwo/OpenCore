#ifndef _OVERLAY_STAGE_H_
#define _OVERLAY_STAGE_H_

// OverlayStage.hpp
// 覆盖层场景的基类

#include "Stage.hpp"

class OverlayStage : public Stage
{
  public:
    OverlayStage() : Stage(overlayStage) {}
};

#endif //_OVERLAY_STAGE_H_