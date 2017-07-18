#pragma once

#include <sqee/setup.hpp>

namespace sts {

//====== Forward Declarations ================================================//

class Entity; class Renderer;

//============================================================================//

class RenderEntity : sq::NonCopyable
{
public: //====================================================//

    RenderEntity(const Entity& entity, const Renderer& renderer);

    virtual ~RenderEntity() = default;

    //--------------------------------------------------------//

    virtual void integrate(float blend) = 0;

    virtual void render_depth() = 0;

    virtual void render_main() = 0;

protected: //=================================================//

    const Entity& entity;
    const Renderer& renderer;
};

//============================================================================//

} // namespace sts
