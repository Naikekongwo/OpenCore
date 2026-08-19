#pragma once

#include "Character.hpp"
#include <vector>

using std::vector;

class CharacterRoom : public UIElement
{
  public:
  private:
    vector<CharaPtr> characters;

    string mainCharacterId;
};