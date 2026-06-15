#include "Asset/PackageManager.hpp"
#include "OpenCore.hpp"
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using std::fstream;

PackageManager::PackageManager(string_view pName)
{
    if (pName.empty())
    {
        LOG("初始化包名称遇到空值");
        throw std::runtime_error("packagename is null.");
    }

    if (!std::filesystem::exists("data//"))
    {
        std::filesystem::create_directory("data//");
    }

    packageName = pName;

    string packageManifestFile;
    packageManifestFile =
        string("data//") + packageName + string("_manifest.txt");

    if (std::filesystem::exists(packageManifestFile))
    {
        std::filesystem::remove(packageManifestFile);
    }

    fstream newManifest;
    newManifest.open(packageManifestFile, std::ios::out);
    newManifest.close();

    LOG("初始化成功，清单文件已保存到 {}", packageManifestFile);
}

bool PackageManager::registerResource(ResourceType rType, string_view name,
                                      string_view filePath)
{
    if (packageName.empty())
    {
        LOG("包名为空，无法确定清单位置");
        return false;
    }

    ResourceNode newBee;

    newBee.rType = rType;
    newBee.name = name;
    newBee.filePath = filePath;

    if (contains(newBee))
        return true;

    resourceManifestBuffer.push_back(newBee);

    string packageManifestFile =
        string("data//") + packageName + string("_manifest.txt");

    fstream manifest(packageManifestFile.c_str(), std::ios::app);

    if (!manifest.good())
    {
        LOG("发生了意外的错误，该清单文件不存在 {}", packageManifestFile);
        return false;
    }

    manifest << newBee.serialize() << std::endl;

    manifest.close();
    return true;
}