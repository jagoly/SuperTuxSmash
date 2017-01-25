#include <iostream>

#include <sqee/gl/Context.hpp>

#include "Cheese.hpp"

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Cheese_Render::Cheese_Render(Renderer& renderer, const Cheese_Fighter& fighter)
    : RenderFighter(renderer, fighter)
{
    //========================================================//

    stuff.MESH_Cheese.load_from_file("fighters/Cheese/mesh");

    //========================================================//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();
    };

    setup_texture(stuff.TX_Cheese_diff, 256u, "fighters/Cheese/textures/Cheese_diff");
    setup_texture(stuff.TX_Cheese_norm, 256u, "fighters/Cheese/textures/Cheese_norm");
    setup_texture(stuff.TX_Cheese_spec, 256u, "fighters/Cheese/textures/Cheese_spec");

    //========================================================//

    stuff.VS_Simple.add_uniform("u_model_mat"); // Mat4F
    stuff.VS_Simple.add_uniform("u_normal_mat"); // Mat3F

    stuff.FS_Cheese.add_uniform("u_colour"); // Vec3F

    //========================================================//

    renderer.shaders.preprocs(stuff.VS_Simple, "fighters/Cheese/Simple_vs");
    renderer.shaders.preprocs(stuff.FS_Cheese, "fighters/Cheese/Cheese_fs");
}

//============================================================================//

Cheese_Render::~Cheese_Render()
{

}

//============================================================================//

void Cheese_Render::render(float progress)
{
    static auto& context = Context::get();

    //std::cout << "rendering a TestDude!" << std::endl;

    //========================================================//

    const auto& fighter = static_cast<const Cheese_Fighter&>(mFighter);
    const auto& previous = fighter.previous, current = fighter.current;

    //const auto& shaders = mRenderer.shaders;
    const auto& camera = mRenderer.camera;

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Replace);

    Vec3F scale; Vec3F colour; float rotation = 0.f;

    //========================================================//

    if (fighter.state.attack == Fighter::State::Attack::None)
    {
        scale = Vec3F(1.f, 1.f, 1.f);
    }

    if (fighter.state.attack == Fighter::State::Attack::Neutral)
    {
        scale = Vec3F(1.f, 1.f, 1.5f);
    }

    //========================================================//

    if (fighter.state.move == Fighter::State::Move::None)
    {
        colour = Vec3F(1.f, 0.f, 0.f);
    }

    if (fighter.state.move == Fighter::State::Move::Walking)
    {
        colour = Vec3F(0.f, 1.f, 0.f);
    }

    if (fighter.state.move == Fighter::State::Move::Dashing)
    {
        colour = Vec3F(0.3f, 0.3f, 0.6f);
    }

    if (fighter.state.move == Fighter::State::Move::Jumping)
    {
        colour = Vec3F(0.f, 0.f, 1.f);
    }

    if (fighter.state.move == Fighter::State::Move::Falling)
    {
        colour = Vec3F(0.1f, 0.1f, 2.f);
    }

    //========================================================//

    if (fighter.state.direction == Fighter::State::Direction::Left)
    {
        rotation = -0.1f;
    }

    if (fighter.state.direction == Fighter::State::Direction::Right)
    {
        rotation = +0.1f;
    }

    //========================================================//

    Vec2F position = maths::mix(previous.position, current.position, progress);
    Mat4F modelMatrix = maths::translate(Mat4F(), Vec3F(position.x, 0.f, position.y));
    modelMatrix = maths::rotate(modelMatrix, Vec3F(0.f, 1.f, 0.f), rotation);
    modelMatrix = maths::scale(modelMatrix, scale);

    Mat3F normalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);

    //========================================================//

    context.use_Shader_Vert(stuff.VS_Simple);

    stuff.VS_Simple.update("u_model_mat", modelMatrix);
    stuff.VS_Simple.update("u_normal_mat", normalMatrix);

    context.bind_VertexArray(stuff.MESH_Cheese.get_vao());

    //========================================================//

    context.bind_Texture(stuff.TX_Cheese_diff, 0u);
    context.bind_Texture(stuff.TX_Cheese_norm, 1u);
    context.bind_Texture(stuff.TX_Cheese_spec, 2u);

    context.use_Shader_Frag(stuff.FS_Cheese);

    stuff.FS_Cheese.update("u_colour", colour);

    stuff.MESH_Cheese.draw_complete();
}
