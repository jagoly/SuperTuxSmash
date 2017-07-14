#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include <game/Game.hpp>

#include <game/fighters/Sara/Actions.hpp>
#include <game/fighters/Sara/Fighter.hpp>

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Sara_Fighter::Sara_Fighter(Game& game, Controller& controller) : Fighter("Sara", game, controller) {}

Sara_Fighter::~Sara_Fighter() = default;

//============================================================================//

void Sara_Fighter::setup()
{
    actions = std::make_unique<Sara_Actions>(*this);

    //========================================================//

    ARMA_Sara.load_bones("fighters/Sara/armature.txt");
    ARMA_Sara.load_rest_pose("fighters/Sara/poses/Rest.txt");

    //========================================================//

    POSE_Rest = ARMA_Sara.make_pose("fighters/Sara/poses/Rest.txt");
    POSE_Stand = ARMA_Sara.make_pose("fighters/Sara/poses/Stand.txt");
    POSE_Jump = ARMA_Sara.make_pose("fighters/Sara/poses/Jump.txt");

    POSE_Act_Neutral = ARMA_Sara.make_pose("fighters/Sara/poses/Act_Neutral.txt");
    POSE_Act_TiltDown = ARMA_Sara.make_pose("fighters/Sara/poses/Act_TiltDown.txt");
    POSE_Act_TiltForward = ARMA_Sara.make_pose("fighters/Sara/poses/Act_TiltForward.txt");
    POSE_Act_TiltUp = ARMA_Sara.make_pose("fighters/Sara/poses/Act_TiltUp.txt");

    ANIM_Walk = ARMA_Sara.make_animation("fighters/Sara/anims/Walk.txt");

    mPosePrevious = mPoseCurrent = POSE_Rest;

    mUbo.create_and_allocate(3840u);

    //========================================================//

    MESH_Sara.load_from_file("fighters/Sara/meshes/Mesh");

    //========================================================//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();

        //gl::TextureParameteri(texture.get_handle(), gl::TEXTURE_BASE_LEVEL, 0);
        //gl::TextureParameteri(texture.get_handle(), gl::TEXTURE_MAX_LEVEL, 3);
    };

    setup_texture(TX_Main_diff, 2048u, "fighters/Sara/textures/Main_diff");
    setup_texture(TX_Main_spec, 2048u, "fighters/Sara/textures/Main_spec");

    setup_texture(TX_Hair_diff, 256u, "fighters/Sara/textures/Hair_diff");
    setup_texture(TX_Hair_norm, 256u, "fighters/Sara/textures/Hair_norm");
    setup_texture(TX_Hair_mask, 256u, "fighters/Sara/textures/Hair_mask");

    TX_Main_spec.set_swizzle_mode('R', 'R', 'R', '1');
    TX_Hair_mask.set_swizzle_mode('0', '0', '0', 'R');

    //========================================================//

    game.renderer->processor.load_vertex(PROG_Main, "fighters/Sara/Sara_vs");
    game.renderer->processor.load_vertex(PROG_Hair, "fighters/Sara/Sara_vs");

    game.renderer->processor.load_fragment(PROG_Main, "fighters/Sara/Main_fs");
    game.renderer->processor.load_fragment(PROG_Hair, "fighters/Sara/Hair_fs");

    PROG_Main.link_program_stages();
    PROG_Hair.link_program_stages();
}

//============================================================================//

void Sara_Fighter::tick()
{
    this->base_tick_Entity();
    this->base_tick_Fighter();

    mPosePrevious = mPoseCurrent;

    //========================================================//

    if (state.move == State::Move::None)
    {
        mAnimationProgress = 0.f;
        mPoseCurrent = POSE_Stand;
    }

    if (state.move == State::Move::Walking)
    {
        // todo: work out exact walk animation speed
        mAnimationProgress += std::abs(mVelocity.x) / 2.f;
        mPoseCurrent = ARMA_Sara.compute_pose(ANIM_Walk, mAnimationProgress);
    }

    if (state.move == State::Move::Dashing)
    {
        // todo: work out exact dash animation speed
        mAnimationProgress += std::abs(mVelocity.x) / 2.f;
        mPoseCurrent = ARMA_Sara.compute_pose(ANIM_Walk, mAnimationProgress);
    }

    if (state.move == State::Move::Jumping)
    {
        mAnimationProgress = 0.f;
        mPoseCurrent = POSE_Jump;
    }

    if (actions->active.type == Actions::Type::Neutral_First)
    {
        mAnimationProgress = 0.f;
        mPoseCurrent = POSE_Act_Neutral;
    }

    if (actions->active.type == Actions::Type::Tilt_Down)
    {
        mAnimationProgress = 0.f;
        mPoseCurrent = POSE_Act_TiltDown;
    }

//    if (actions->active.type == Actions::Type::Tilt_Forward)
//    {
//        mAnimationProgress = 0.f;
//        mPoseCurrent = POSE_Act_TiltForward;
//    }

    if (actions->active.type == Actions::Type::Tilt_Up)
    {
        mAnimationProgress = 0.f;
        mPoseCurrent = POSE_Act_TiltUp;
    }

}

//============================================================================//

void Sara_Fighter::integrate(float blend)
{
    const auto& camera = game.renderer->camera;

    //========================================================//

    const auto blendPose = ARMA_Sara.blend_poses(mPosePrevious, mPoseCurrent, blend);

    const auto data = ARMA_Sara.compute_ubo_data(blendPose);
    mUbo.update(0u, uint(data.size()) * 48u, data.data());

    //========================================================//

    QuatF rotation { 0.f, 0.f, 0.f, 1.f };
    Vec3F scale { 1.f, 1.f, 1.f };

    //========================================================//

    if (state.direction == State::Direction::Left)
        rotation = QuatF(0.f, 0.f, -0.25f);

    if (state.direction == State::Direction::Right)
        rotation = QuatF(0.f, 0.f, +0.25f);

    //========================================================//

    const Vec2F position = maths::mix(mPreviousPosition, mCurrentPosition, blend);
    const Mat4F modelMatrix = maths::transform(Vec3F(position.x, 0.f, position.y), rotation, scale);

    mFinalMatrix = camera.projMatrix * camera.viewMatrix * modelMatrix;
    mNormalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);
}

void Sara_Fighter::render_depth()
{
    static auto& context = Context::get();

    const auto& shaders = game.renderer->shaders;

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::LessEqual);
    context.set_state(Context::Depth_Test::Replace);

    //========================================================//

    context.bind_VertexArray(MESH_Sara.get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    //========================================================//

    shaders.PROG_Depth_SkellySolid.update(0, mFinalMatrix);
    context.bind_Program(shaders.PROG_Depth_SkellySolid);

    MESH_Sara.draw_partial(0u);

    //========================================================//

    context.set_state(Context::Cull_Face::Disable);

    shaders.PROG_Depth_SkellyPunch.update(0, mFinalMatrix);
    context.bind_Program(shaders.PROG_Depth_SkellyPunch);

    context.bind_Texture(TX_Hair_mask, 0u);
    MESH_Sara.draw_partial(1u);

    context.set_state(Context::Cull_Face::Back);
}

void Sara_Fighter::render_main()
{
    static auto& context = Context::get();

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Compare::Equal);
    context.set_state(Context::Depth_Test::Keep);

    //========================================================//

    PROG_Main.update(0, mFinalMatrix);
    PROG_Main.update(1, mNormalMatrix);

    PROG_Hair.update(0, mFinalMatrix);
    PROG_Hair.update(1, mNormalMatrix);

    PROG_Hair.update(3, Vec3F(0.35f, 0.35f, 0.25f));

    //========================================================//

    context.bind_VertexArray(MESH_Sara.get_vao());
    context.bind_UniformBuffer(mUbo, 2u);

    //========================================================//

    context.bind_Texture(TX_Main_diff, 0u);
    context.bind_Texture(TX_Main_spec, 2u);

    context.bind_Program(PROG_Main);
    MESH_Sara.draw_partial(0u);

    //========================================================//

    context.set_state(Context::Cull_Face::Disable);

    context.bind_Texture(TX_Hair_diff, 0u);
    context.bind_Texture(TX_Hair_norm, 1u);

    context.bind_Program(PROG_Hair);
    MESH_Sara.draw_partial(1u);

    context.set_state(Context::Cull_Face::Back);
}
