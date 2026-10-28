#pragma once

#include "Character.hpp"
#include <vector>

using std::vector;

class CharacterRoom : public OpenUI::UIElement
{
  public:
  private:
    vector<CharaPtr> characters;

    string mainCharacterId;
};