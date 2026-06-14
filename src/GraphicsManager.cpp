// GfxCoe.cpp
// 负责 GFX_CORE 的方法实现

#include "OpenCore.hpp"
#include <SDL3/SDL_Log.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstddef>

GraphicsManager &GraphicsManager::getInstance()
{
    // 单例 ： 仅执行一次
    static GraphicsManager instance;
    // 这个instance的对象在应用的整个生命周期中只会执行一次。
    return instance;
}

bool GraphicsManager::Init()
{
    // 图形核心的初始化方法

    // 初始化SDL 失败即返回假
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        return false;
    }

    if (!TTF_Init())
    {
        LOG("TTF_Init Error: {}", SDL_GetError());
        return false;
    }

    // 其他平台的初始化方法
    if (!SDL_CreateWindowAndRenderer("OpenCore Window", 1280, 720, 0, &window,
                                     &renderer))
    {
        return false;
    }

    SDL_SetWindowResizable(window, true);

    // 初始化成功
    return true;
}

void GraphicsManager::refreshWindowProperties()
{
    auto gameInfo = OpenEngine::getInstance().getGameInfo();

    if (!gameInfo)
    {
        LOG("GraphicsManager::Window Encountered a null pointer for game "
            "info!");
        return;
    }

    std::string title = OpenEngine::getInstance().getGameInfo()->gameName;

    if (gameInfo->beta == true)
    {
        title = title + " : " +
                std::to_string(static_cast<int>(gameInfo->version_major)) +
                "." +
                std::to_string(static_cast<int>(gameInfo->version_minor)) +
                std::string(" Beta with OpenCore ") +
                std::to_string(static_cast<int>(ENGINE_VERSION_MAJOR)) + "." +
                std::to_string(static_cast<int>(ENGINE_VERSION_MINOR));
    }

    SDL_SetWindowTitle(window, title.c_str());

    OpenCoreManagers::SetManager.setTargetWidth(
        gameInfo->_graphicsInfo.resolutionWidth,
        gameInfo->_graphicsInfo.resolutionHeight);

    SDL_SetRenderLogicalPresentation(renderer,
                                     gameInfo->_graphicsInfo.resolutionWidth,
                                     gameInfo->_graphicsInfo.resolutionHeight,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    TargetWindowWidth = gameInfo->_graphicsInfo.resolutionWidth;
    TargetWindowHeight = gameInfo->_graphicsInfo.resolutionHeight;

    if (!gameInfo->_graphicsInfo.resizable)
    {
        SDL_SetWindowResizable(window, false);
    }
    else if (gameInfo->_graphicsInfo.keepRatio)
    {
        int windowWidth = 0;
        int windowHeight = 0;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        windowWidth =
            (TargetWindowWidth * 1.0f / TargetWindowHeight) * windowHeight;
        SDL_SetWindowSize(window, windowWidth, windowHeight);
    }
}

void GraphicsManager::CleanUp()
{
    if (!renderer)
        LOG("failed to destroy a null renderer.");
    else
        SDL_DestroyRenderer(renderer);

    if (!window)
        LOG("failed to destroy a null window.");
    else
        SDL_DestroyWindow(window);

    LOG("has reseted the scale system.");

    LOG("manage to quit the SDL.");
    SDL_Quit();
}

int GraphicsManager::Draw(SDL_Texture *texture, const Rect *srcRect,
                          const Rect *dstRect, const double angle,
                          const Point *center)
{
    SDL_Rect source;
    SDL_Rect destination;
    SDL_Point point;

    if (!texture)
    {
        LOG("Encountered a empty texture.");
        return -1;
    }
    if (srcRect)
    {
        source = *srcRect;
    }
    if (dstRect)
    {
        destination = *dstRect;
    }
    if (center)
    {
        point = *center;
    }

    SDL_FRect srcF, dstF;
    SDL_FPoint centerF;
    if (srcRect)
        srcF = *srcRect;
    if (dstRect)
        dstF = *dstRect;
    if (center)
        centerF = *center;

    return SDL_RenderTextureRotated(
        renderer, texture, (srcRect) ? &srcF : nullptr,
        (dstRect) ? &dstF : nullptr, angle, (center) ? &centerF : nullptr,
        SDL_FLIP_NONE);
}

int GraphicsManager::DrawSDLGeometry(SDL_Texture *texture,
                                     const SDL_Vertex *vertices,
                                     int num_vertices, const int *indices,
                                     int num_indices)
{
    return SDL_RenderGeometry(renderer, texture, vertices, num_vertices,
                              indices, num_indices);
}

SDL_Texture *GraphicsManager::createTexture(uint16_t w, uint16_t h)
{
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET, w, h);
    // 启用透明混合
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

int GraphicsManager::setRenderTarget(SDL_Texture *texture)
{
    if (SDL_SetRenderTarget(renderer, texture) != 0)
    {
        LOG("Failed to set render target: {}", SDL_GetError());
        return -1;
    }

    if (texture)
    {
        // 可选：只在需要清理时才清理
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
    }

    return 0;
}

void GraphicsManager::FillRect(const Rect &rect, const SDL_Color &color)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_FRect fRect = rect;
    SDL_RenderFillRect(renderer, &fRect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
}
