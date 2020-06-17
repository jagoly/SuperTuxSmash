#include "fighters/Mario_Render.hpp"

#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Mario_Render::Mario_Render(Renderer& renderer, const Mario_Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.create_and_allocate(sizeof(Mario_Render::mCharacterBlock));

    //--------------------------------------------------------//

    ResourceCaches& cache = renderer.resources;

    MESH_Body = cache.meshes.acquire("assets/fighters/Mario/meshes/Body");
    MESH_Eyes = cache.meshes.acquire("assets/fighters/Mario/meshes/Eyes");
    MESH_HairHat = cache.meshes.acquire("assets/fighters/Mario/meshes/HairHat");
    MESH_Head = cache.meshes.acquire("assets/fighters/Mario/meshes/Head");

    TX_BodyA_diff = cache.textures.acquire("assets/fighters/Mario/textures/BodyA_diff");
    TX_BodyB_diff = cache.textures.acquire("assets/fighters/Mario/textures/BodyB_diff");

    sq::ProgramKey programKey;
    programKey.vertexPath = "fighters/BasicFighter_vs";
    programKey.fragmentDefines = "#define OPT_TEX_DIFFUSE";
    programKey.fragmentPath = "fighters/BasicFighter_fs";
    PROG_Main = cache.programs.acquire(programKey);
}

//============================================================================//

void Mario_Render::integrate(float blend)
{
    //constexpr const Mat4F scaleMatrix = maths::scale(Mat4F(), {0.1f, 0.1f, 0.1f});
    const Mat4F modelMatrix = fighter.interpolate_model_matrix(blend);

    mCharacterBlock.matrix = renderer.get_camera().get_combo_matrix() * modelMatrix;
    mCharacterBlock.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_view_matrix() * modelMatrix));

    auto& bones = mCharacterBlock.bones;
    fighter.interpolate_bone_matrices(blend, bones.data(), bones.size());

    mUbo.update(0u, mCharacterBlock);
}

//============================================================================//

void Mario_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    //--------------------------------------------------------//

    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);
    context.bind_VertexArray(MESH_Body->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    context.set_state(Context::Cull_Face::Back);
    context.bind_Program(shaders.Depth_FighterSolid);
    context.bind_VertexArray(MESH_Body->get_vao());
    MESH_Body->draw_complete();
    context.bind_VertexArray(MESH_Eyes->get_vao());
    MESH_Eyes->draw_complete();
    context.bind_VertexArray(MESH_HairHat->get_vao());
    MESH_HairHat->draw_complete();
    context.bind_VertexArray(MESH_Head->get_vao());
    MESH_Head->draw_complete();
}

//============================================================================//

void Mario_Render::render_main()
{
    auto& context = renderer.context;

    //--------------------------------------------------------//

    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);
    context.bind_UniformBuffer(mUbo, 2u);

    context.set_state(Context::Cull_Face::Back);
    context.bind_Program(PROG_Main.get());

    context.bind_Texture(TX_BodyA_diff.get(), 0u);

    context.bind_VertexArray(MESH_Body->get_vao());
    MESH_Body->draw_partial(0u);
    context.bind_VertexArray(MESH_HairHat->get_vao());
    MESH_HairHat->draw_complete();
    context.bind_VertexArray(MESH_Head->get_vao());
    MESH_Head->draw_complete();

    context.bind_Texture(TX_BodyB_diff.get(), 0u);

    context.bind_VertexArray(MESH_Body->get_vao());
    MESH_Body->draw_partial(1u);
    MESH_Body->draw_partial(2u);
    context.bind_VertexArray(MESH_Eyes->get_vao());
    MESH_Eyes->draw_complete();
}