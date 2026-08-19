#pragma once

/**
 * @brief CharacterManager.hpp
 * @details 角色管理器
 */

#include <string>
#include <vector>

using std::string;
using std::string_view;
using std::vector;

struct CharacterGraphicsInfo
{
    // 在进行表情合成时的x,y偏移
    float blend_offset_x = 0.5f;
    float blend_offset_y = 0.5f;

    // 人物的高度(单位:m)
    float heightFactor = 1.8f;
};

struct CharacterAudioInfo
{
};

struct Character
{
    // 角色的名称/代号
    string name;

    // 角色的图形属性
    CharacterGraphicsInfo chGInfo;
    // 角色的音频选项
    CharacterAudioInfo chAInfo;
};

class CharacterManager
{
  private:
};