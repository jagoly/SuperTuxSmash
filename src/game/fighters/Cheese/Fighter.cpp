#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include <game/Misc.hpp>

#include <game/fighters/Cheese/Fighter.hpp>

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Cheese_Fighter::Cheese_Fighter() : Fighter("Cheese") {}

Cheese_Fighter::~Cheese_Fighter() = default;

//============================================================================//

void Cheese_Fighter::setup()
{
    SQASSERT(mController != nullptr, "");
    SQASSERT(mRenderer != nullptr, "");

    //========================================================//

    misc::load_actions_from_json(*this);

    //========================================================//

    MESH_Cheese.load_from_file("fighters/Cheese/Mesh");

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

    VS_Cheese.add_uniform("u_model_mat"); // Mat4F
    VS_Cheese.add_uniform("u_normal_mat"); // Mat3F
    FS_Cheese.add_uniform("u_colour"); // Vec3F

    //========================================================//

    mRenderer->shaders.preprocs(VS_Cheese, "fighters/Cheese/Cheese_vs");
    mRenderer->shaders.preprocs(FS_Cheese, "fighters/Cheese/Cheese_fs");
}

//============================================================================//

void Cheese_Fighter::tick()
{
    this->impl_tick_base();
}

//============================================================================//

void Cheese_Fighter::render()
{
    static auto& context = Context::get();

    const auto& progress = mRenderer->progress;
    const auto& camera = mRenderer->camera;

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Replace);

    QuatF rotation { 0.f, 0.f, 0.f, 1.f };
    Vec3F scale { 1.f, 1.f, 1.f };
    Vec3F colour { 0.f, 0.f, 0.f };

    //========================================================//

    if (state.action == State::Action::None)
        scale = Vec3F(1.f, 1.f, 1.f);

    if (state.action == State::Action::Neutral)
        scale = Vec3F(1.f, 1.f, 1.5f);

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
    const Mat3F normalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);

    //========================================================//

    context.use_Shader_Vert(VS_Cheese);

    VS_Cheese.update("u_model_mat", modelMatrix);
    VS_Cheese.update("u_normal_mat", normalMatrix);

    context.bind_VertexArray(MESH_Cheese.get_vao());

    //========================================================//

    context.bind_Texture(TX_Cheese_diff, 0u);
    context.bind_Texture(TX_Cheese_norm, 1u);
    context.bind_Texture(TX_Cheese_spec, 2u);

    context.use_Shader_Frag(FS_Cheese);

    FS_Cheese.update("u_colour", colour);

    MESH_Cheese.draw_complete();
}
