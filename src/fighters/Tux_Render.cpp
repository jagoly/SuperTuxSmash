#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include "fighters/Tux_Render.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Tux_Render::Tux_Render(Renderer& renderer, const Tux_Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.create_and_allocate(3840u);

    //--------------------------------------------------------//

    ResourceCaches& cache = renderer.resources;

    MESH_Tux = cache.meshes.acquire("fighters/Tux/meshes/Mesh");

    TX_Tux_diff = cache.textures.acquire("fighters/Tux/textures/Tux_diff");

    //--------------------------------------------------------//

    renderer.processor.load_vertex(PROG_Tux, "fighters/Tux/Tux_vs");
    renderer.processor.load_fragment(PROG_Tux, "fighters/Tux/Tux_fs");
    PROG_Tux.link_program_stages();
}

//============================================================================//

void Tux_Render::integrate(float blend)
{
    const Mat4F modelMatrix = fighter.interpolate_model_matrix(blend);

    mFinalMatrix = renderer.get_camera().get_combo_matrix() * modelMatrix;
    mNormalMatrix = maths::normal_matrix(renderer.get_camera().get_view_matrix() * modelMatrix);

    const auto& boneMatrices = fighter.interpolate_bone_matrices(blend);

    mUbo.update(0u, uint(boneMatrices.size()) * 48u, boneMatrices.data());
}

//============================================================================//

void Tux_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    //--------------------------------------------------------//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    context.bind_VertexArray(MESH_Tux->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    shaders.Depth_SkellySolid.update(0, mFinalMatrix);
    context.bind_Program(shaders.Depth_SkellySolid);

    MESH_Tux->draw_complete();
}

//============================================================================//

void Tux_Render::render_main()
{
    auto& context = renderer.context;

    //--------------------------------------------------------//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    context.bind_VertexArray(MESH_Tux->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    context.bind_Texture(TX_Tux_diff.get(), 0u);
    PROG_Tux.update(3, Vec3F(0.4f, 0.4f, 0.4f));

    PROG_Tux.update(0, mFinalMatrix);
    PROG_Tux.update(1, mNormalMatrix);
    context.bind_Program(PROG_Tux);

    MESH_Tux->draw_complete();
}
