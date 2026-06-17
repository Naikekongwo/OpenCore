#pragma once

/**
 * @brief 资源注册信息
 * @param packageOnly 是否只允许运行在资源包模式
 * @param keepStructureWhenPackaging
 * 是否在打包时保留文件结构（若否，则解包无法获得完整目录结构）
 * @param textureResampleFactor
 * 资源缩放因数，默认为1.0f，适用于向其他平台编译时的资源缩放
 */
struct ResourceInfo
{
    bool  packageOnly;
    bool  keepStructureWhenPackaging = false;
    float textureResampleFactor      = 1.0f;
};