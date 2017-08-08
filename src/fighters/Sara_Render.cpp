#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include "fighters/Sara_Render.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Sara_Render::Sara_Render(const Renderer& renderer, const Sara_Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.create_and_allocate(3840u);

    //--------------------------------------------------------//

    MESH_Sara.load_from_file("fighters/Sara/meshes/Mesh", true);

    //--------------------------------------------------------//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();
    };

    setup_texture(TX_Main_diff, 2048u, "fighters/Sara/textures/Main_diff");
    setup_texture(TX_Main_spec, 2048u, "fighters/Sara/textures/Main_spec");

    setup_texture(TX_Hair_diff, 256u, "fighters/Sara/textures/Hair_diff");
    setup_texture(TX_Hair_norm, 256u, "fighters/Sara/textures/Hair_norm");
    setup_texture(TX_Hair_mask, 256u, "fighters/Sara/textures/Hair_mask");

    TX_Main_spec.set_swizzle_mode('R', 'R', 'R', '1');
    TX_Hair_mask.set_swizzle_mode('0', '0', '0', 'R');

    //--------------------------------------------------------//

    renderer.processor.load_vertex(PROG_Main, "fighters/Sara/Sara_vs");
    renderer.processor.load_vertex(PROG_Hair, "fighters/Sara/Sara_vs");

    renderer.processor.load_fragment(PROG_Main, "fighters/Sara/Main_fs");
    renderer.processor.load_fragment(PROG_Hair, "fighters/Sara/Hair_fs");

    PROG_Main.link_program_stages();
    PROG_Hair.link_program_stages();
}

//============================================================================//

void Sara_Render::integrate(float blend)
{
    const Mat4F modelMatrix = fighter.interpolate_model_matrix(blend);

    mFinalMatrix = renderer.camera.projMatrix * renderer.camera.viewMatrix * modelMatrix;
    mNormalMatrix = maths::normal_matrix(renderer.camera.viewMatrix * modelMatrix);

    const auto& boneMatrices = fighter.interpolate_bone_matrices(blend);

    mUbo.update(0u, uint(boneMatrices.size()) * 48u, boneMatrices.data());
}

//============================================================================//

void Sara_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    //--------------------------------------------------------//

    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);
    context.bind_VertexArray(MESH_Sara.get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

        context.set_state(Context::Cull_Face::Back);
        shaders.Depth_SkellySolid.update(0, mFinalMatrix);
        context.bind_Program(shaders.Depth_SkellySolid);
        MESH_Sara.draw_partial(0u);

        context.set_state(Context::Cull_Face::Disable);
        context.bind_Texture(TX_Hair_mask, 0u);
        shaders.Depth_SkellyPunch.update(0, mFinalMatrix);
        context.bind_Program(shaders.Depth_SkellyPunch);
        MESH_Sara.draw_partial(1u);
}

//============================================================================//

void Sara_Render::render_main()
{
    auto& context = renderer.context;

    //--------------------------------------------------------//

    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);
    context.bind_VertexArray(MESH_Sara.get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

        context.set_state(Context::Cull_Face::Back);
        context.bind_Texture(TX_Main_diff, 0u);
        context.bind_Texture(TX_Main_spec, 2u);
        PROG_Main.update(0, mFinalMatrix);
        PROG_Main.update(1, mNormalMatrix);
        context.bind_Program(PROG_Main);
        MESH_Sara.draw_partial(0u);

        context.set_state(Context::Cull_Face::Disable);
        context.bind_Texture(TX_Hair_diff, 0u);
        context.bind_Texture(TX_Hair_norm, 1u);
        PROG_Hair.update(0, mFinalMatrix);
        PROG_Hair.update(1, mNormalMatrix);
        PROG_Hair.update(3, Vec3F(0.35f, 0.35f, 0.25f));
        context.bind_Program(PROG_Hair);
        MESH_Sara.draw_partial(1u);
}
