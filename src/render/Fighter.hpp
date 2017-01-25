#pragma once

#include <game/Fighter.hpp>
#include <render/Renderer.hpp>

namespace sts {

//============================================================================//

class RenderFighter : sq::NonCopyable
{
public:

    //========================================================//

    RenderFighter(Renderer& renderer, const Fighter& fighter);

    virtual ~RenderFighter() = default;

    //========================================================//

    virtual void render(float progress) = 0;

protected:

    //========================================================//

    Renderer& mRenderer;
    const Fighter& mFighter;
};

//============================================================================//

} // namespace sts
