#ifndef _BASE_STAGE_H_
#define _BASE_STAGE_H_

// BaseStage.hpp
// 底层场景的基类

#include "World/Stage/Stage.hpp"

class BaseStage : public Stage
{
  public:
    BaseStage(Timer *timer, StageManager *sController,
              StageType type = baseStage)
        : Stage(timer, sController, type)
    {
    }
};

#endif //_BASE_STAGE_H_