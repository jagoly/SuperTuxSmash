#pragma once

#include <sqee/setup.hpp>

namespace sts {

//====== Forward Declarations ================================================//

class Entity; class Renderer; class Options;

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

    template <class T> constexpr auto& entity_cast()
    { return static_cast<const T&>(entity); }

    //--------------------------------------------------------//

    const Entity& entity;
    const Renderer& renderer;
};

//============================================================================//

} // namespace sts
