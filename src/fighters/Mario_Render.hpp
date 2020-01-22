#pragma once

#include <sqee/gl/UniformBuffer.hpp>

#include "render/RenderObject.hpp"
#include "fighters/Mario_Fighter.hpp"

//============================================================================//

namespace sts {

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
    MeshHandle MESH_Eyes;
    MeshHandle MESH_HairHat;
    MeshHandle MESH_Head;

    TextureHandle TX_BodyA_diff;
    TextureHandle TX_BodyB_diff;
    TextureHandle TX_EyeIris_diff;

    ProgramHandle PROG_Main;

    //--------------------------------------------------------//

    CharacterBlock<56> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Mario_Fighter& fighter;
};

} // namespace sts
