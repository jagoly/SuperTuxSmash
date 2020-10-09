#pragma once

#include "main/Resources.hpp"

#include "render/RenderObject.hpp"
#include "render/UniformBlocks.hpp"

#include <sqee/gl/FixedBuffer.hpp>

namespace sts {

//============================================================================//

class RenderFighter final : public RenderObject
{
public: //====================================================//

    RenderFighter(Renderer& renderer, const Fighter& fighter);

    void integrate(float blend) override;

    void render_opaque() const override;

    void render_transparent() const override;

private: //===================================================//

    const Fighter& fighter;

    sq::FixedBuffer mUbo;

    bool mConditionFlinch = false;
};

//============================================================================//

} // namespace sts
