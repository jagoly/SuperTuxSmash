#include "stages/TestZone_Render.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

#include <sqee/gl/Context.hpp>

using sq::Context;
using namespace sts;

//============================================================================//

TestZone_Render::TestZone_Render(Renderer& renderer, const TestZone_Stage& stage)
    : RenderObject(renderer), stage(stage)
{
    ResourceCaches& cache = renderer.resources;

    MESH_Mesh = cache.meshes.acquire("assets/stages/TestZone/meshes/Mesh");

    TEX_Diff = cache.textures.acquire("assets/stages/TestZone/textures/Diffuse");

    sq::ProgramKey programKey;
    programKey.vertexPath = "stages/StaticMesh_vs";
    programKey.fragmentDefines = "#define OPT_TEX_DIFFUSE";
    programKey.fragmentPath = "BasicModel_fs";
    PROG_Main = cache.programs.acquire(programKey);
}

//============================================================================//

void TestZone_Render::integrate(float /*blend*/)
{
    mFinalMatrix = renderer.get_camera().get_combo_matrix();
    mNormalMatrix = maths::normal_matrix(renderer.get_camera().get_view_matrix());
}

//============================================================================//

void TestZone_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    context.bind_VertexArray(MESH_Mesh->get_vao());

    shaders.Depth_StaticSolid.update(0, mFinalMatrix);
    context.bind_Program(shaders.Depth_StaticSolid);

    MESH_Mesh->draw_complete();
}

//============================================================================//

void TestZone_Render::render_main()
{
    auto& context = renderer.context;

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    context.bind_VertexArray(MESH_Mesh->get_vao());

    context.bind_Texture(TEX_Diff.get(), 0u);

    PROG_Main->update(0, mFinalMatrix);
    PROG_Main->update(1, mNormalMatrix);
    PROG_Main->update(3, Vec3F(0.5f, 0.5f, 0.5f));
    context.bind_Program(PROG_Main.get());

    MESH_Mesh->draw_complete();
}
