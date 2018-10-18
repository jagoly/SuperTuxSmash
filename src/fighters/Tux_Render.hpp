#pragma once

#include <sqee/gl/UniformBuffer.hpp>

#include "render/RenderObject.hpp"
#include "fighters/Tux_Fighter.hpp"

//============================================================================//

namespace sts {

class Tux_Render final : public RenderObject
{
public: //====================================================//

    Tux_Render(Renderer& renderer, const Tux_Fighter& fighter);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

private: //===================================================//

    MeshHandle MESH_Tux;

    TextureHandle TX_Tux_diff;
    TextureHandle TX_Tux_spec;

    ProgramHandle PROG_Tux;

    //--------------------------------------------------------//

    CharacterBlock<21> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Tux_Fighter& fighter;
};

} // namespace sts
