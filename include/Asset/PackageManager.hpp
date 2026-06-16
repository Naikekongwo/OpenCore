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
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using std::fstream;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::vector;

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
    string       filePath;
    int          startIndex = 0;
    int          endIndex = 0;

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
        auto firstComma = line.find(',');
        auto secondComma = line.find(',', firstComma + 1);
        auto thirdComma = line.find(',', secondComma + 1);
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
               filePath == other.filePath;
    }
};

/**
 * @brief 资源包管理器类
 * @details OpenCore 26.1版本引入的新资源管理器类
 */
class PackageManager final
{
  public:
    explicit PackageManager(string_view pName);
    ~PackageManager() = default;

    /**
     * @brief 设置资源包的前缀名称
     *
     * @param pName
     */
    void setPackageName(string_view pName) { this->packageName = pName; }

    /**
     * @brief 资源管理器的更新方法，用于判断资源过期等内容
     *
     */
    void onUpdate();
    void onDestroy();

    bool registerResource(ResourceType rType, string_view name,
                          string_view filePath);
    bool registerResource(ResourceNode resource);
    bool registerResources(std::initializer_list<ResourceNode> resources);

  private:
    string packageName;
    bool   packedMode = false;

    vector<ResourceNode> resourceManifestBuffer;

    static std::filesystem::path getManifestPath(string_view packageName,
                                                 bool        packed);

    bool contains(ResourceNode target);

    bool extractManifest(string_view packagePath);
    bool generatePackage(string_view manifestPath);
    bool enablePackage(string_view packagePath);
};