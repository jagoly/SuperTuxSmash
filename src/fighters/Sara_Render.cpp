#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include "fighters/Sara_Render.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Sara_Render::Sara_Render(Renderer& renderer, const Sara_Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.create_and_allocate(sizeof(Sara_Render::mCharacterBlock));

    //--------------------------------------------------------//

    ResourceCaches& cache = renderer.resources;

    MESH_Sara = cache.meshes.acquire("assets/fighters/Sara/meshes/Mesh");

    TX_Main_diff = cache.textures.acquire("assets/fighters/Sara/textures/Main_diff");
    TX_Main_spec = cache.textures.acquire("assets/fighters/Sara/textures/Main_spec");

    TX_Hair_mask = cache.textures.acquire("assets/fighters/Sara/textures/Hair_mask");
    TX_Hair_diff = cache.textures.acquire("assets/fighters/Sara/textures/Hair_diff");
    TX_Hair_norm = cache.textures.acquire("assets/fighters/Sara/textures/Hair_norm");
    TX_Hair_spec = cache.textures.acquire("assets/fighters/Sara/textures/Hair_spec");

    sq::ProgramKey programKey;
    programKey.vertexPath = "fighters/BasicFighter_vs";
    programKey.fragmentPath = "fighters/BasicFighter_fs";

    programKey.fragmentDefines = "";
    PROG_Main = cache.programs.acquire(programKey);

    programKey.fragmentDefines = "#define OPT_TEX_NORMAL";
    PROG_Hair = cache.programs.acquire(programKey);
}

//============================================================================//

void Sara_Render::integrate(float blend)
{
    const Mat4F modelMatrix = fighter.interpolate_model_matrix(blend);

    mCharacterBlock.matrix = renderer.get_camera().get_combo_matrix() * modelMatrix;
    mCharacterBlock.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_view_matrix() * modelMatrix));

    auto& bones = mCharacterBlock.bones;
    fighter.interpolate_bone_matrices(blend, bones.data(), bones.size());

    mUbo.update(0u, mCharacterBlock);
}

//============================================================================//

void Sara_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    //--------------------------------------------------------//

    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);
    context.bind_VertexArray(MESH_Sara->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

        context.set_state(Context::Cull_Face::Back);
        context.bind_Program(shaders.Depth_FighterSolid);
        MESH_Sara->draw_partial(0u);

        context.set_state(Context::Cull_Face::Disable);
        context.bind_Texture(TX_Hair_mask.get(), 0u);
        context.bind_Program(shaders.Depth_FighterPunch);
        MESH_Sara->draw_partial(1u);
}

//============================================================================//

void Sara_Render::render_main()
{
    auto& context = renderer.context;

    //--------------------------------------------------------//

    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);
    context.bind_VertexArray(MESH_Sara->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

        context.set_state(Context::Cull_Face::Back);
        context.bind_Texture(TX_Main_diff.get(), 0u);
        context.bind_Texture(TX_Main_spec.get(), 2u);
        context.bind_Program(PROG_Main.get());
        MESH_Sara->draw_partial(0u);

        context.set_state(Context::Cull_Face::Disable);
        context.bind_Texture(TX_Hair_diff.get(), 0u);
        context.bind_Texture(TX_Hair_norm.get(), 1u);
        context.bind_Texture(TX_Hair_spec.get(), 2u);
        context.bind_Program(PROG_Hair.get());
        MESH_Sara->draw_partial(1u);
}
