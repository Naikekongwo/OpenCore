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
    string name;
    string filePath;

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
        return typeStr + "," + name + "," + filePath;
    }

    /// 从一行字符串反序列化
    static ResourceNode deserialize(string_view line)
    {
        ResourceNode node;
        // 按逗号分割
        auto firstComma = line.find(',');
        auto secondComma = line.find(',', firstComma + 1);

        string typeStr(line.substr(0, firstComma));
        node.name = line.substr(firstComma + 1, secondComma - firstComma - 1);
        node.filePath = line.substr(secondComma + 1);

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
    void onDestory();

    bool registerResource(ResourceType rType, string_view name,
                          string_view filePath);
    bool registerResource(ResourceNode newBee);
    bool registerResources(std::initializer_list<ResourceNode> newBees);

  private:
    string packageName;

    vector<ResourceNode> resourceManifestBuffer;

    bool contains(ResourceNode newBee)
    {
        bool contain_flag = false;
        for (auto entry : resourceManifestBuffer)
        {
            if (entry == newBee)
            {
                contain_flag = true;
            }
        }

        // 此举是为了判断缓存中的
        if (!contain_flag)
        {
            string packageManifestFile;
            packageManifestFile =
                string("data//") + packageName + string("_manifest.txt");
            fstream maifest(packageManifestFile.c_str(), std::ios::in);
            if (!maifest.good())
            {
                // 文件不存在，创建新的空清单文件
                maifest.open(packageManifestFile, std::ios::out);
                maifest.close();
                return false;
            }

            string working_line;
            while (std::getline(maifest, working_line))
            {
                if (!working_line.empty())
                {
                    ResourceNode node = ResourceNode::deserialize(working_line);
                    if (node == newBee)
                    {
                        // 其存在于清单文件，但是在内存中不存在
                        // 可以视作其是超时的文件，使其重新回来
                        resourceManifestBuffer.push_back(newBee);
                        return true;
                    }
                }
            }
            maifest.close();
        }
        // 此举是为了查找本地清单中的

        return contain_flag;
    }
};