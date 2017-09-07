#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include "render/Renderer.hpp"

#include "stages/TestZone_Render.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

TestZone_Render::TestZone_Render(const Renderer& renderer, const TestZone_Stage& stage)
    : RenderObject(renderer), stage(stage)
{
    MESH_Mesh.load_from_file("stages/TestZone/Mesh", true);

    //--------------------------------------------------------//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();
    };

    setup_texture(TEX_Diff, 2048u, "stages/TestZone/Diffuse");

    TEX_Diff.set_swizzle_mode('R', 'G', 'B', '1');

    //--------------------------------------------------------//

    renderer.processor.load_vertex(PROG_Main, "stages/TestZone/Main_vs");
    renderer.processor.load_fragment(PROG_Main, "stages/TestZone/Main_fs");
    PROG_Main.link_program_stages();
}

//============================================================================//

void TestZone_Render::integrate(float blend)
{
    mFinalMatrix = renderer.camera.projMatrix * renderer.camera.viewMatrix;
    mNormalMatrix = maths::normal_matrix(renderer.camera.viewMatrix);
}

//============================================================================//

void TestZone_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    //--------------------------------------------------------//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    context.bind_VertexArray(MESH_Mesh.get_vao());

    shaders.Depth_SimpleSolid.update(0, mFinalMatrix);
    context.bind_Program(shaders.Depth_SimpleSolid);

    MESH_Mesh.draw_complete();
}

//============================================================================//

void TestZone_Render::render_main()
{
    auto& context = renderer.context;

    //--------------------------------------------------------//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    context.bind_VertexArray(MESH_Mesh.get_vao());

    context.bind_Texture(TEX_Diff, 0u);

    PROG_Main.update(0, mFinalMatrix);
    PROG_Main.update(1, mNormalMatrix);
    PROG_Main.update(3, Vec3F(0.5f, 0.5f, 0.5f));
    context.bind_Program(PROG_Main);

    MESH_Mesh.draw_complete();
}
