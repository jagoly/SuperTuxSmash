#include "fighters/Sara_Render.hpp"
#include "fighters/Sara_Fighter.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

#include <sqee/gl/Context.hpp>

using sq::Context;
using namespace sts;

//============================================================================//

Sara_Render::Sara_Render(Renderer& renderer, const Sara_Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.create_and_allocate(sizeof(Sara_Render::mCharacterBlock));

    MESH_Sara = renderer.meshes.acquire("fighters/Sara/meshes/Mesh");

    TEX_Main_diff = renderer.textures.acquire("fighters/Sara/textures/Main_diff");
    TEX_Main_spec = renderer.textures.acquire("fighters/Sara/textures/Main_spec");

    TEX_Hair_mask = renderer.textures.acquire("fighters/Sara/textures/Hair_mask");
    TEX_Hair_diff = renderer.textures.acquire("fighters/Sara/textures/Hair_diff");
    TEX_Hair_norm = renderer.textures.acquire("fighters/Sara/textures/Hair_norm");
    TEX_Hair_spec = renderer.textures.acquire("fighters/Sara/textures/Hair_spec");

    sq::ProgramKey programKey;
    programKey.vertexPath = "fighters/BasicFighter_vs";
    programKey.fragmentPath = "fighters/BasicFighter_fs";

    programKey.fragmentDefines = "#define OPT_TEX_DIFFUSE\n#define OPT_TEX_SPECULAR";
    PROG_Main = renderer.programs.acquire(programKey);

    programKey.fragmentDefines = "#define OPT_TEX_DIFFUSE\n#define OPT_TEX_NORMAL\n#define OPT_TEX_SPECULAR";
    PROG_Hair = renderer.programs.acquire(programKey);
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

    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);
    context.bind_VertexArray(MESH_Sara->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    context.set_state(Context::Cull_Face::Back);
    context.bind_Program(renderer.PROG_Depth_FighterSolid);
    MESH_Sara->draw_partial(0u);

    context.set_state(Context::Cull_Face::Disable);
    context.bind_Texture(TEX_Hair_mask.get(), 0u);
    context.bind_Program(renderer.PROG_Depth_FighterPunch);
    MESH_Sara->draw_partial(1u);
}

//============================================================================//

void Sara_Render::render_main()
{
    auto& context = renderer.context;

    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);
    context.bind_VertexArray(MESH_Sara->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    context.set_state(Context::Cull_Face::Back);
    context.bind_Texture(TEX_Main_diff.get(), 0u);
    context.bind_Texture(TEX_Main_spec.get(), 2u);
    context.bind_Program(PROG_Main.get());
    MESH_Sara->draw_partial(0u);

    context.set_state(Context::Cull_Face::Disable);
    context.bind_Texture(TEX_Hair_diff.get(), 0u);
    context.bind_Texture(TEX_Hair_norm.get(), 1u);
    context.bind_Texture(TEX_Hair_spec.get(), 2u);
    context.bind_Program(PROG_Hair.get());
    MESH_Sara->draw_partial(1u);
}
