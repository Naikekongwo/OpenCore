/**
 * @file ElementManager.hpp
 * @author Naikekkongwo
 * @brief UI控件管理器的头文件
 * @version 1.0
 * @date 2025-08-03
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef _ELEMENTMANANGER_H_
#define _ELEMENTMANANGER_H_

#include "Runtime/Graphics/IDrawableObject/IDrawableObject.hpp"

#include "Runtime/Graphics/UI/BaseBackground.hpp"
#include "Runtime/Graphics/UI/Button.hpp"
#include "Runtime/Graphics/UI/CheckBox.hpp"
#include "Runtime/Graphics/UI/FrameCounter.hpp"
#include "Runtime/Graphics/UI/ImageBoard.hpp"
#include "Runtime/Graphics/UI/ItemContainer.hpp"
#include "Runtime/Graphics/UI/MultiImageBoard.hpp"
#include "Runtime/Graphics/UI/Scrollbar.hpp"
#include "Runtime/Graphics/UI/Symbol.hpp"
#include "Runtime/Graphics/UI/TextArea.hpp"
#include "Runtime/Graphics/UI/TypeWriter.hpp"

#include "Runtime/Graphics/Sprite/HealthBar.hpp"
#include "Runtime/Graphics/Sprite/ItemSprite.hpp"
#include "Runtime/Graphics/Sprite/Tile.h"
#include "Runtime/Graphics/Sprite/Waterrect.hpp"

#include "Runtime/Animation/Manager/AnimationManager.hpp"
#include "Runtime/Graphics/IDrawableObject/Sprite.hpp"

#include <memory>
#include <vector>

/**
 * @brief 元素管理器类
 *
 */
class ElementManager
{
  public:
    ~ElementManager() { onDestroy(); }

    // 基础方法

    void handlEvents(SDL_Event &event, float totalTime);

    void onUpdate(float totalTime);

    /**
     * @brief UI控件的渲染调用联动接口
     * @todo 需要进行原生化（优化元素成员类直接为UIElement），后更名Draw()方法
     */
    void onRender();

    void onDestroy();

    bool removeElement(const std::string &id);

    bool PushElement(unique_ptr<IDrawableObject> element);

    IDrawableObject *find(const std::string &id) const;

    unique_ptr<IDrawableObject> getElement(const std::string &id);

    template <typename Func> void forEachElement(Func &&func)
    {
        for (auto &elem : Elements)
        {
            if (elem)
                func(elem);
        }
    }

    template <typename Predicate> bool any(Predicate &&pred) const
    {
        for (const auto &elem : Elements)
        {
            if (elem && pred(elem))
                return true;
        }
        return false;
    }

  private:
    // 储存子类的表
    vector<unique_ptr<IDrawableObject>> Elements;
};
#endif //_ELEMENTMANANGER_H_