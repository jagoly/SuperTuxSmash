#include "fighters/Tux_Render.hpp"
#include "fighters/Tux_Fighter.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

#include <sqee/gl/Context.hpp>

using sq::Context;
using namespace sts;

//============================================================================//

Tux_Render::Tux_Render(Renderer& renderer, const Tux_Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.create_and_allocate(sizeof(Tux_Render::mCharacterBlock));

    MESH_Tux = renderer.meshes.acquire("fighters/Tux/meshes/Mesh");

    TEX_Tux_diff = renderer.textures.acquire("fighters/Tux/textures/Tux_diff");
    TEX_Tux_spec = renderer.textures.acquire("fighters/Tux/textures/Tux_spec");

    sq::ProgramKey programKey;
    programKey.fragmentDefines = "#define OPT_TEX_DIFFUSE\n#define OPT_TEX_SPECULAR";
    programKey.vertexPath = "fighters/BasicFighter_vs";
    programKey.fragmentPath = "fighters/BasicFighter_fs";
    PROG_Tux = renderer.programs.acquire(programKey);
}

//============================================================================//

void Tux_Render::integrate(float blend)
{
    const Mat4F modelMatrix = fighter.interpolate_model_matrix(blend);

    mCharacterBlock.matrix = renderer.get_camera().get_combo_matrix() * modelMatrix;
    mCharacterBlock.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_view_matrix() * modelMatrix));

    auto& bones = mCharacterBlock.bones;
    fighter.interpolate_bone_matrices(blend, bones.data(), bones.size());

    mUbo.update(0u, mCharacterBlock);
}

//============================================================================//

void Tux_Render::render_depth()
{
    auto& context = renderer.context;

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    context.bind_VertexArray(MESH_Tux->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    context.bind_Program(renderer.PROG_Depth_FighterSolid);

    MESH_Tux->draw_complete();
}

//============================================================================//

void Tux_Render::render_main()
{
    auto& context = renderer.context;

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    context.bind_VertexArray(MESH_Tux->get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    context.bind_Texture(TEX_Tux_diff.get(), 0u);
    context.bind_Texture(TEX_Tux_spec.get(), 2u);

    context.bind_Program(PROG_Tux.get());

    MESH_Tux->draw_complete();
}
