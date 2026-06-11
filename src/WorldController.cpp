#include "Runtime/Gameplay/WorldController/WorldController.hpp"
#include "OpenCore.hpp"
#include <optional>

void WorldController::onEnter()
{
    if (status == WorldControllerStatus::Registered)
    {
        m_homelessItem = std::nullopt;
        status = WorldControllerStatus::Ready;
    }
}

bool WorldController::generateMapManager() { return true; }

bool WorldController::spawnMapEntities() { return true; }

bool WorldController::generateTheMan() { return true; }

void WorldController::onUpdate(float totalTime) { (void)totalTime; }

PhysicalProperties *WorldController::queryPhysicalProp(short EntityIndex)
{
    if (Entities.contains(EntityIndex))
    {
        return &Entities.at(EntityIndex)->getPhysicalProperties();
    }

    return nullptr;
}

optional<BlockInfo> WorldController::queryBlockInfo(int gx, int gy)
{
    if (mapManager->isReady())
    {
        return mapManager->getBlockInfo(gx, gy);
    }

    return std::nullopt;
}

Entity *WorldController::getEntityByID(short id)
{
    if (!Entities.contains(id))
        return nullptr;

    return Entities.at(id).get();
}

void WorldController::getEntities(vector<Entity *> &list)
{
    list.clear();
    list.reserve(Entities.size());
    for (const auto &[id, ePtr] : Entities)
    {
        list.push_back(ePtr.get());
    }
}

BackPtr WorldController::getBackpackByEntityID(short id)
{
    auto entity = getEntityByID(id);

    if (!entity)
    {
        LOG("尝试获取的实体对象并不存在, ID {}", id);
        return nullptr;
    }

    auto backpack = entity->getBackpack();
    return backpack;
}

BackPtr WorldController::getBackpackByID(short id)
{
    if (!containers.contains(id))
    {
        LOG("所请求的背包并不存在， ID {}", id);
        return nullptr;
    }

    return containers.at(id);
}

bool WorldController::regMovement(short entityID, Vec3 Speed)
{
    auto entity = getEntityByID(entityID);
    if (!entity)
        return false;

    auto &pProperties = entity->getPhysicalProperties();

    pProperties.setDesiredVelocity(Speed);

    LOG("设置成功");

    return true;
}

bool WorldController::pushHomelessItem(short backpackID, short backpackIndex)
{
    // 首先验证背包有效性
    if (!containers.contains(backpackID))
        return false;
    if (containers.at(backpackID)->getCapacity() <= backpackIndex)
        return false;

    auto itemOpt = containers.at(backpackID)->getItem(backpackIndex);
    if (!itemOpt.has_value() || !itemOpt->item.has_value())
        return false;

    ItemExchangeRecord record{itemOpt.value(), backpackID, 0, false};

    m_homelessItem = record;

    // 清空原槽位
    containers.at(backpackID)->removeItem(backpackIndex);
    return true;
}

optional<ItemInstance> WorldController::popHomelessItem()
{
    if (!m_homelessItem.has_value())
        return std::nullopt;

    ItemInstance instance = m_homelessItem->instance;
    m_homelessItem = std::nullopt;
    return instance;
}

optional<ItemInfo> WorldController::queryHomelessItemInfo()
{
    if (m_homelessItem.has_value())
    {
        return m_homelessItem->instance.item->getItemInfo();
    }

    return std::nullopt;
}

void WorldController::giveUpHomelessItem()
{
    if (m_homelessItem.has_value())
    {
        if (containers.contains(m_homelessItem->srcBackpackID))
        {
            containers.at(m_homelessItem->srcBackpackID)
                ->addItem(m_homelessItem->instance.item->getTypeID(),
                          m_homelessItem->instance.count);
        }
    }

    m_homelessItem = std::nullopt;
}