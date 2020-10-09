#pragma once

#include "main/Resources.hpp"

#include "render/RenderObject.hpp"

#include <sqee/gl/FixedBuffer.hpp>

namespace sts {

//============================================================================//

class RenderStage final : public RenderObject
{
public: //====================================================//

    RenderStage(Renderer& renderer, const Stage& stage);

    void integrate(float blend) override;

    void render_opaque() const override;

    void render_transparent() const override;

private: //===================================================//

    const Stage& stage;

    sq::FixedBuffer mUbo;
};

//============================================================================//

} // namespace sts
