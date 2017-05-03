#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include <game/Game.hpp>

#include <game/fighters/Cheese/Actions.hpp>
#include <game/fighters/Cheese/Fighter.hpp>

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Cheese_Fighter::Cheese_Fighter(Game& game) : Fighter("Cheese", game) {}

Cheese_Fighter::~Cheese_Fighter() = default;

//============================================================================//

void Cheese_Fighter::setup()
{
    actions = std::make_unique<Cheese_Actions>(*this);

    //========================================================//

    MESH_Cheese.load_from_file("fighters/Cheese/meshes/Mesh");

    //========================================================//

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

    //========================================================//

    VS_Cheese.add_uniform("u_final_mat"); // Mat4F
    VS_Cheese.add_uniform("u_normal_mat"); // Mat3F
    FS_Cheese.add_uniform("u_colour"); // Vec3F

    //========================================================//

    game.renderer->shaders.preprocs(VS_Cheese, "fighters/Cheese/Cheese_vs");
    game.renderer->shaders.preprocs(FS_Cheese, "fighters/Cheese/Cheese_fs");
}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->impl_tick_base();
}

//============================================================================//

void Cheese_Fighter::integrate()
{
    const auto& progress = game.renderer->progress;
    const auto& camera = game.renderer->camera;

    //========================================================//

    QuatF rotation { 0.f, 0.f, 0.f, 1.f };
    Vec3F scale { 1.f, 1.f, 1.f };
    Vec3F colour { 0.f, 0.f, 0.f };

    //========================================================//

    if (state.move == State::Move::None)
        colour = Vec3F(1.f, 0.f, 0.f);

    if (state.move == State::Move::Walking)
        colour = Vec3F(0.f, 1.f, 0.f);

    if (state.move == State::Move::Dashing)
        colour = Vec3F(0.3f, 0.3f, 0.6f);

    if (state.move == State::Move::Jumping)
        colour = Vec3F(0.f, 0.f, 1.f);

    if (state.move == State::Move::Falling)
        colour = Vec3F(0.1f, 0.1f, 2.f);

    //========================================================//

    if (state.direction == State::Direction::Left)
        rotation = QuatF(0.f, -0.1f, 0.f);

    if (state.direction == State::Direction::Right)
        rotation = QuatF(0.f, +0.1f, 0.f);

    //========================================================//

    const Vec2F position = maths::mix(previous.position, current.position, progress);
    const Mat4F modelMatrix = maths::transform(Vec3F(position.x, 0.f, position.y), rotation, scale);

    mFinalMatrix = camera.projMatrix * camera.viewMatrix * modelMatrix;
    mNormalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);

    FS_Cheese.update("u_colour", colour);
}

//============================================================================//

void Cheese_Fighter::render_depth()
{
    static auto& context = Context::get();

    const auto& shaders = game.renderer->shaders;

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    //========================================================//

    shaders.VS_Depth_Simple.update("u_final_mat", mFinalMatrix);
    context.use_Shader_Vert(shaders.VS_Depth_Simple);

    context.bind_VertexArray(MESH_Cheese.get_vao());

    //========================================================//

    context.disable_shader_stage_fragment();
    MESH_Cheese.draw_complete();
}

//============================================================================//

void Cheese_Fighter::render_main()
{
    static auto& context = Context::get();

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    //========================================================//

    VS_Cheese.update("u_final_mat", mFinalMatrix);
    VS_Cheese.update("u_normal_mat", mNormalMatrix);

    //========================================================//

    context.use_Shader_Vert(VS_Cheese);

    context.bind_VertexArray(MESH_Cheese.get_vao());

    //========================================================//

    context.bind_Texture(TX_Cheese_diff, 0u);
    context.bind_Texture(TX_Cheese_norm, 1u);
    context.bind_Texture(TX_Cheese_spec, 2u);

    context.use_Shader_Frag(FS_Cheese);

    MESH_Cheese.draw_complete();
}
