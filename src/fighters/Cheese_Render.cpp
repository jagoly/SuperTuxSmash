#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include "render/Renderer.hpp"

#include "fighters/Cheese_Fighter.hpp"
#include "fighters/Cheese_Render.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Cheese_Render::Cheese_Render(const Entity& entity, const Renderer& renderer)
    : RenderEntity(entity, renderer)
{
    MESH_Cheese.load_from_file("fighters/Cheese/meshes/Mesh", true);

    //--------------------------------------------------------//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();
    };

    setup_texture(TX_Cheese_diff, 256u, "fighters/Cheese/textures/Cheese_diff");
    setup_texture(TX_Cheese_norm, 256u, "fighters/Cheese/textures/Cheese_norm");
    setup_texture(TX_Cheese_spec, 256u, "fighters/Cheese/textures/Cheese_spec");

    //--------------------------------------------------------//

    renderer.processor.load_vertex(PROG_Cheese, "fighters/Cheese/Cheese_vs");
    renderer.processor.load_fragment(PROG_Cheese, "fighters/Cheese/Cheese_fs");

    PROG_Cheese.link_program_stages();
}

//============================================================================//

void Cheese_Render::integrate(float blend)
{
    auto& fighter = static_cast<const Cheese_Fighter&>(entity);

    //--------------------------------------------------------//

    const Vec2F position = maths::mix(fighter.mPreviousPosition, fighter.mCurrentPosition, blend);
    const QuatF rotation = QuatF(0.f, 0.f, -0.1f * float(fighter.state.direction));

    const Mat4F modelMatrix = maths::transform(Vec3F(position, 0.f), rotation, Vec3F(1.f));

    mFinalMatrix = renderer.camera.projMatrix * renderer.camera.viewMatrix * modelMatrix;
    mNormalMatrix = maths::normal_matrix(renderer.camera.viewMatrix * modelMatrix);

    //--------------------------------------------------------//

    PROG_Cheese.update(4, fighter.mColour);
}

//============================================================================//

void Cheese_Render::render_depth()
{
    auto& context = renderer.context;
    auto& shaders = renderer.shaders;

    //--------------------------------------------------------//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    context.bind_VertexArray(MESH_Cheese.get_vao());

    shaders.Depth_SimpleSolid.update(0, mFinalMatrix);
    context.bind_Program(shaders.Depth_SimpleSolid);

    MESH_Cheese.draw_complete();
}

//============================================================================//

void Cheese_Render::render_main()
{
    auto& context = renderer.context;

    //--------------------------------------------------------//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    context.bind_VertexArray(MESH_Cheese.get_vao());

    context.bind_Texture(TX_Cheese_diff, 0u);
    context.bind_Texture(TX_Cheese_norm, 1u);
    context.bind_Texture(TX_Cheese_spec, 2u);

    PROG_Cheese.update(0, mFinalMatrix);
    PROG_Cheese.update(1, mNormalMatrix);
    context.bind_Program(PROG_Cheese);

    MESH_Cheese.draw_complete();
}
