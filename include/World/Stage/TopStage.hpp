#ifndef _TOP_STAGE_H_
#define _TOP_STAGE_H_

// TopStage.hpp
// 顶层场景

#include "World/Stage/Stage.hpp"

class TopStage : public Stage
{
  public:
    TopStage() : Stage(topStage) {}
};

#endif //_TOP_STAGE_H_
