#include <iostream>

#include <sqee/gl/Context.hpp>

#include "Sara.hpp"

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Sara_Render::Sara_Render(Renderer& renderer, const Sara_Fighter& fighter)
    : RenderFighter(renderer, fighter)
{
    stuff.MESH_Sara.load_from_file("fighters/Sara/mesh");

    //========================================================//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();
    };

    setup_texture(stuff.TX_Main_diff, 2048u, "fighters/Sara/textures/Main_diff");
    setup_texture(stuff.TX_Main_spec, 2048u, "fighters/Sara/textures/Main_spec");

    setup_texture(stuff.TX_Hair_diff, 512u, "fighters/Sara/textures/Hair_diff");
    setup_texture(stuff.TX_Hair_norm, 512u, "fighters/Sara/textures/Hair_norm");
    setup_texture(stuff.TX_Hair_mask, 512u, "fighters/Sara/textures/Hair_mask");

    stuff.TX_Main_spec.set_swizzle_mode('R', 'R', 'R', '1');
    stuff.TX_Hair_mask.set_swizzle_mode('0', '0', '0', 'R');

    //========================================================//

    stuff.VS_Simple.add_uniform("u_model_mat"); // Mat4F
    stuff.VS_Simple.add_uniform("u_normal_mat"); // Mat3F
    stuff.FS_Hair.add_uniform("u_specular"); // Vec3F

    renderer.shaders.preprocs(stuff.VS_Simple, "fighters/Sara/Simple_vs");
    renderer.shaders.preprocs(stuff.FS_Main, "fighters/Sara/Main_fs");
    renderer.shaders.preprocs(stuff.FS_Hair, "fighters/Sara/Hair_fs");
}

Sara_Render::~Sara_Render()
{

}

//============================================================================//

void Sara_Render::render(float progress)
{
    static auto& context = Context::get();

    //std::cout << "rendering a TestDude!" << std::endl;

    //========================================================//

    const auto& fighter = static_cast<const Sara_Fighter&>(mFighter);
    const auto& previous = fighter.previous, current = fighter.current;

    //const auto& shaders = mRenderer.shaders;
    const auto& camera = mRenderer.camera;

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Replace);

    Vec3F scale; Vec3F tint; float rotation = 0.f;

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
        tint = Vec3F(1.f, 0.f, 0.f);
    }

    if (fighter.state.move == Fighter::State::Move::Walking)
    {
        tint = Vec3F(0.f, 1.f, 0.f);
    }

    if (fighter.state.move == Fighter::State::Move::Dashing)
    {
        tint = Vec3F(0.3f, 0.3f, 0.6f);
    }

    if (fighter.state.move == Fighter::State::Move::Jumping)
    {
        tint = Vec3F(0.f, 0.f, 1.f);
    }

    if (fighter.state.move == Fighter::State::Move::Falling)
    {
        tint = Vec3F(0.1f, 0.1f, 2.f);
    }

    //========================================================//

    if (fighter.state.direction == Fighter::State::Direction::Left)
    {
        rotation = -0.25f;
    }

    if (fighter.state.direction == Fighter::State::Direction::Right)
    {
        rotation = +0.25f;
    }

    //========================================================//

    Vec2F position = maths::mix(previous.position, current.position, progress);
    Mat4F modelMatrix = maths::translate(Mat4F(), Vec3F(position.x, 0.f, position.y));
    modelMatrix = maths::rotate(modelMatrix, Vec3F(0.f, 0.f, 1.f), rotation);
    modelMatrix = maths::scale(modelMatrix, scale);

    Mat3F normalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);

    //========================================================//

    context.use_Shader_Vert(stuff.VS_Simple);

    stuff.VS_Simple.update("u_model_mat", modelMatrix);
    stuff.VS_Simple.update("u_normal_mat", normalMatrix);

    context.bind_VertexArray(stuff.MESH_Sara.get_vao());

    //========================================================//

    context.bind_Texture(stuff.TX_Main_diff, 0u);
    context.bind_Texture(stuff.TX_Main_spec, 2u);

    context.use_Shader_Frag(stuff.FS_Main);

    stuff.MESH_Sara.draw_partial(0u);

    //========================================================//

    context.bind_Texture(stuff.TX_Hair_diff, 0u);
    context.bind_Texture(stuff.TX_Hair_norm, 1u);
    context.bind_Texture(stuff.TX_Hair_mask, 3u);

    context.use_Shader_Frag(stuff.FS_Hair);

    stuff.FS_Hair.update("u_specular", Vec3F(0.5f, 0.4f, 0.1f));

    stuff.MESH_Sara.draw_partial(1u);
}
