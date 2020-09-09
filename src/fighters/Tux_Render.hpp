#pragma once

#include "render/RenderObject.hpp"

#include "caches/MeshCache.hpp"
#include "caches/ProgramCache.hpp"
#include "caches/TextureCache.hpp"

#include "render/UniformBlocks.hpp"

#include <sqee/gl/UniformBuffer.hpp>

namespace sts {

class Tux_Fighter;

//============================================================================//

class Tux_Render final : public RenderObject
{
public: //====================================================//

    Tux_Render(Renderer&, const Tux_Fighter&);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

private: //===================================================//

    MeshHandle MESH_Tux;

    TextureHandle TEX_Tux_diff;
    TextureHandle TEX_Tux_spec;

    ProgramHandle PROG_Tux;

    //--------------------------------------------------------//

    CharacterBlock<24> mCharacterBlock;

    sq::UniformBuffer mUbo;

    //--------------------------------------------------------//

    const Tux_Fighter& fighter;
};

//============================================================================//

} // namespace sts
