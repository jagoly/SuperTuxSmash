#pragma once

#include "render/RenderObject.hpp"

#include "caches/MeshCache.hpp"
#include "caches/ProgramCache.hpp"
#include "caches/TextureCache.hpp"

#include "render/UniformBlocks.hpp"

#include <sqee/gl/UniformBuffer.hpp>

namespace sts {

class Mario_Fighter;

//============================================================================//

class Mario_Render final : public RenderObject
{
public: //====================================================//

    Mario_Render(Renderer&, const Mario_Fighter&);

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
    MeshHandle MESH_HeadHurt;

    TextureHandle TEX_BodyA_diff;
    TextureHandle TEX_BodyB_diff;
    TextureHandle TEX_EyeWhite_diff;
    TextureHandle TEX_EyeIris_diff;
    TextureHandle TEX_EyeIris_mask;
    TextureHandle TEX_EyeHurt_diff;

    ProgramHandle PROG_Main;

    //--------------------------------------------------------//

    CharacterBlock<56> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Mario_Fighter& fighter;
};

//============================================================================//

} // namespace sts
