#pragma once

#include "render/RenderObject.hpp"

#include "render/ResourceCaches.hpp"
#include "render/UniformBlocks.hpp"

#include <sqee/gl/UniformBuffer.hpp>

namespace sts {

class Sara_Fighter;

//============================================================================//

class Sara_Render final : public RenderObject
{
public: //====================================================//

    Sara_Render(Renderer& renderer, const Sara_Fighter& fighter);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

private: //===================================================//

    MeshHandle MESH_Sara;

    TextureHandle TX_Main_diff;
    TextureHandle TX_Main_spec;

    TextureHandle TX_Hair_mask;
    TextureHandle TX_Hair_diff;
    TextureHandle TX_Hair_norm;
    TextureHandle TX_Hair_spec;

    ProgramHandle PROG_Main;
    ProgramHandle PROG_Hair;

    //--------------------------------------------------------//

    CharacterBlock<65> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Sara_Fighter& fighter;
};

//============================================================================//

} // namespace sts
