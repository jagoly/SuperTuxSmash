#pragma once

#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>
#include <sqee/gl/Program.hpp>

#include <sqee/render/Mesh.hpp>

#include "render/RenderObject.hpp"

#include "fighters/Sara_Fighter.hpp"

//============================================================================//

namespace sts {

class Sara_Render final : public RenderObject
{
public: //====================================================//

    Sara_Render(Renderer& renderer, const Sara_Fighter& fighter);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

    //ParticleSet::Refs get_particle_sets() override;

private: //===================================================//

    MeshHandle MESH_Sara;

    TextureHandle TX_Main_diff;
    TextureHandle TX_Main_spec;

    TextureHandle TX_Hair_mask;
    TextureHandle TX_Hair_diff;
    TextureHandle TX_Hair_norm;

    sq::Program PROG_Main;
    sq::Program PROG_Hair;

    //--------------------------------------------------------//

    sq::UniformBuffer mUbo;

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;

    //--------------------------------------------------------//

    const Sara_Fighter& fighter;
};

} // namespace sts
