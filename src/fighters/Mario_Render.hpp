#pragma once

#include "render/RenderObject.hpp"

#include "render/ResourceCaches.hpp"
#include "render/UniformBlocks.hpp"

#include <sqee/gl/UniformBuffer.hpp>

namespace sts {

class Mario_Fighter;

//============================================================================//

class Mario_Render final : public RenderObject
{
public: //====================================================//

    Mario_Render(Renderer& renderer, const Mario_Fighter& fighter);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

private: //===================================================//

    MeshHandle MESH_Body;
    MeshHandle MESH_HairHat;
    MeshHandle MESH_Head;
    MeshHandle MESH_EyesWhite;
    MeshHandle MESH_EyesIris;

    TextureHandle TX_BodyA_diff;
    TextureHandle TX_BodyB_diff;
    TextureHandle TX_EyeWhite_diff;
    TextureHandle TX_EyeIris_diff;
    TextureHandle TX_EyeIris_mask;

    ProgramHandle PROG_Main;

    //--------------------------------------------------------//

    CharacterBlock<54> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Mario_Fighter& fighter;
};

//============================================================================//

} // namespace sts
