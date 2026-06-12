/**
 * @file OpenCore.hpp
 * @author Naikekongwo
 * @brief OpenCore引擎的头文件
 * @version 1.0
 * @date 2025-07-26
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef _OPENCORE_H_
#define _OPENCORE_H_

enum DebugFlags
{
    DEBUG_OFF = 0,
    DEBUG_WATERWAVE = 1 << 0,  // 0x01
    DEBUG_MAIN = 1 << 1,       // 0x02
    DEBUG_PASS_VIDEO = 1 << 2, // 0x04
    DEBUG_COPYRIGHT = 1 << 3
};

constexpr int DEBUG_MODE = DEBUG_COPYRIGHT | DEBUG_MAIN;

#include "Core/Macros.hpp"
#include "Core/Timer.hpp"

#include "Asset/ResourceManager.hpp"
#include "Core/Info/GameInfo.hpp"
#include "Core/Math/OpenCore_Spiral.hpp"
#include "Core/Math/OpenCore_Vec3.hpp"
#include "Core/Math/OpenCore_Wave.hpp"

#include "Runtime/Config/SettingsManager.hpp"
#include "Runtime/Graphics/Manager/GraphicsManager.hpp"
#include "Runtime/Graphics/Manager/TextureMeta.hpp"

#include "Core/Helpers/Debugger.hpp"

#include "Core/Event/EventManager.hpp"

#include "Core/Thread/ThreadManager.hpp"
#include "World/Helpers/SequentialPipeline.hpp"

/**
 * @brief 负责内核部分的单例管理器类
 *
 */
namespace OpenCoreManagers
{
inline ThreadManager &ThrManager = ThreadManager::getInstance();
inline ResourceManager &ResManager = ResourceManager::getInstance();
inline EventManager &EvtManager = EventManager::GetInstance();
inline GraphicsManager &GFXManager = GraphicsManager::getInstance();

inline SettingsManager &SetManager = SettingsManager::getInstance();
inline TextureMetaManager &TexMetaManager = TextureMetaManager::getInstance();
} // namespace OpenCoreManagers

#include "Runtime/Animation/AnimationPipeline.hpp"
#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include "Runtime/Graphics/Configurator/DrawableConfigurator.hpp"
#include "Runtime/Graphics/Manager/ElementManager.hpp"
#include "World/Stage/StageManager.hpp"

/**
 * @brief 创建材质的模板
 *
 * @tparam T
 * @param id
 * @param layer
 * @param texID
 * @param frameX
 * @param frameY
 * @return unique_ptr<T>
 */
template <typename T>
unique_ptr<T> UI(const std::string &id, uint8_t layer, short texID,
                 short frameX, short frameY);

#include "Runtime/Graphics/Factory/UIFactory.inl"

unique_ptr<Texture> MakeTexture(uint8_t xCount, uint8_t yCount, short texId);

#include "Runtime/Graphics/Factory/TextureFactory.inl"

#include <memory>

using std::make_shared;
using std::shared_ptr;
using std::string;

/**
 * @brief OpenCore的引擎主类
 *
 */
class OpenEngine final
{
  public:
    // 单例的获取函数
    static OpenEngine &getInstance();

    bool Run();
    bool Initialize();
    bool MainLoop();
    bool CleanUp();

    // 注册游戏类
    bool GameRegistry(unique_ptr<GameInfo> gameInfo);

    // 工具函数
    Timer *getTimer() const noexcept { return timer.get(); }
    StageManager *getStageController() const noexcept
    {
        return sController.get();
    }

    GameInfo *getGameInfo() { return gameInfo.get(); }

  private:
    unique_ptr<GameInfo> gameInfo = std::make_unique<GameInfo>();
    unique_ptr<StageManager> sController;
    unique_ptr<Timer> timer;
};

#endif //_OPENCORE_H_