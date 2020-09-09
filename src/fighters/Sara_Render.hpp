#pragma once

#include "render/RenderObject.hpp"

#include "caches/MeshCache.hpp"
#include "caches/ProgramCache.hpp"
#include "caches/TextureCache.hpp"

#include "render/UniformBlocks.hpp"

#include <sqee/gl/UniformBuffer.hpp>

namespace sts {

class Sara_Fighter;

//============================================================================//

class Sara_Render final : public RenderObject
{
public: //====================================================//

    Sara_Render(Renderer&, const Sara_Fighter&);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

private: //===================================================//

    MeshHandle MESH_Sara;

    TextureHandle TEX_Main_diff;
    TextureHandle TEX_Main_spec;

    TextureHandle TEX_Hair_mask;
    TextureHandle TEX_Hair_diff;
    TextureHandle TEX_Hair_norm;
    TextureHandle TEX_Hair_spec;

    ProgramHandle PROG_Main;
    ProgramHandle PROG_Hair;

    //--------------------------------------------------------//

    CharacterBlock<67> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Sara_Fighter& fighter;
};

//============================================================================//

} // namespace sts
