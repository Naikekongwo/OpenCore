/**
 * @file PackageManager.hpp
 * @author your name (you@domain.com)
 * @brief 资源包管理器类
 * @version 0.1
 * @date 2026-06-15
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "Core/Info/ResourceInfo.hpp"
#include "Core/Timer.hpp"

using std::fstream;
using std::initializer_list;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;
using std::filesystem::path;

enum ResourceType
{
    RscTexture,
    RscMusic,
    RscFont
};

/**
 * @brief 资源节点，用于保存索引文件
 *
 */
struct ResourceNode
{
    ResourceType rType;
    string       name;
    string       filePath;       // 此参数在生成资源包时舍弃
    int          startIndex = 0; // 此参数不由外部显式注册
    int          endIndex   = 0; // 此参数不由外部显式注册
    float expireTime = 0.0f;     // 此参数与序列化、反序列化无关（不储存到本地）

    /// 序列化为一行字符串（CSV格式）
    string serialize() const
    {
        string typeStr;
        switch (rType)
        {
        case RscTexture:
            typeStr = "Texture";
            break;
        case RscMusic:
            typeStr = "Music";
            break;
        case RscFont:
            typeStr = "Font";
            break;
        }
        return typeStr + "," + name + "," + filePath + "," +
               std::to_string(startIndex) + "," + std::to_string(endIndex);
    }

    /// 从一行字符串反序列化
    static ResourceNode deserialize(string_view line)
    {
        ResourceNode node;
        // 按逗号分割
        auto firstComma  = line.find(',');
        auto secondComma = line.find(',', firstComma + 1);
        auto thirdComma  = line.find(',', secondComma + 1);
        auto fourthComma = line.find(',', thirdComma + 1);

        string typeStr(line.substr(0, firstComma));
        node.name = line.substr(firstComma + 1, secondComma - firstComma - 1);
        node.filePath =
            line.substr(secondComma + 1, thirdComma - secondComma - 1);
        node.startIndex = std::stoi(
            string(line.substr(thirdComma + 1, fourthComma - thirdComma - 1)));
        node.endIndex = std::stoi(string(line.substr(fourthComma + 1)));

        if (typeStr == "Texture")
            node.rType = RscTexture;
        else if (typeStr == "Music")
            node.rType = RscMusic;
        else if (typeStr == "Font")
            node.rType = RscFont;

        return node;
    }

    bool operator==(const ResourceNode &other) const
    {
        return rType == other.rType && name == other.name &&
               filePath == other.filePath && startIndex == other.startIndex &&
               endIndex == other.endIndex;
    }
};

/**
 * @brief 资源包管理器类
 * @details OpenCore 26.1版本引入的新资源管理器类
 */
class PackageManager final
{
  public:
    explicit PackageManager(string_view  pName,
                            ResourceInfo resInfo = ResourceInfo{});
    ~PackageManager() = default;

    /**
     * @brief 设置资源包的前缀名称
     *
     * @param pName
     */
    void setPackageName(string_view pName) { this->packageName = pName; }

    bool onEnter();

    /**
     * @brief 资源管理器的更新方法，用于判断资源过期等内容
     *
     */
    void onUpdate();
    void onDestroy();

    bool registerResource(ResourceType rType, string_view name,
                          string_view filePath);
    bool registerResource(ResourceNode resource);
    bool registerResources(initializer_list<ResourceNode> resources);

  private:
    string packageName;
    bool   packedMode = false;
    Timer *timer      = nullptr;

    ResourceInfo resourceInfo;

    vector<ResourceNode> resourceManifestBuffer;

    static constexpr float EVICT_TTL = 10.0f; // 进行资源删除轮询的时间

    void evictStaleEntries();

    static path getManifestPath(string_view packageName, bool packed);

    bool contains(ResourceNode target, bool nameOnly = false);

    bool generatePackage(const path &manifestPath, bool cleanup = true);
};