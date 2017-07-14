#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include <game/Game.hpp>

#include <game/fighters/Cheese/Actions.hpp>
#include <game/fighters/Cheese/Fighter.hpp>

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Cheese_Fighter::Cheese_Fighter(Game& game, Controller& controller) : Fighter("Cheese", game, controller) {}

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

    game.renderer->processor.load_vertex(PROG_Cheese, "fighters/Cheese/Cheese_vs");
    game.renderer->processor.load_fragment(PROG_Cheese, "fighters/Cheese/Cheese_fs");

    PROG_Cheese.link_program_stages();
}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->base_tick_Entity();
    this->base_tick_Fighter();
}

//============================================================================//

void Cheese_Fighter::integrate(float blend)
{
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

    const Vec2F position = maths::mix(mPreviousPosition, mCurrentPosition, blend);
    const Mat4F modelMatrix = maths::transform(Vec3F(position.x, 0.f, position.y), rotation, scale);

    mFinalMatrix = camera.projMatrix * camera.viewMatrix * modelMatrix;
    mNormalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);

    PROG_Cheese.update(4, colour);
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

    shaders.PROG_Depth_SimpleSolid.update(0, mFinalMatrix);
    context.bind_Program(shaders.PROG_Depth_SimpleSolid);

    context.bind_VertexArray(MESH_Cheese.get_vao());

    //========================================================//

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

    PROG_Cheese.update(0, mFinalMatrix);
    PROG_Cheese.update(1, mNormalMatrix);

    //========================================================//

    context.bind_VertexArray(MESH_Cheese.get_vao());

    //========================================================//

    context.bind_Texture(TX_Cheese_diff, 0u);
    context.bind_Texture(TX_Cheese_norm, 1u);
    context.bind_Texture(TX_Cheese_spec, 2u);

    context.bind_Program(PROG_Cheese);
    MESH_Cheese.draw_complete();
}
