#pragma once

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

//============================================================================//

namespace sts {

class RenderObject : sq::NonCopyable
{
public: //====================================================//

    RenderObject(const Renderer& renderer);

    virtual ~RenderObject() = default;

    //--------------------------------------------------------//

    virtual void integrate(float blend) = 0;

    virtual void render_depth() = 0;

    virtual void render_main() = 0;

    virtual void render_alpha() = 0;

protected: //=================================================//

    const Renderer& renderer;
};

} // namespace sts
