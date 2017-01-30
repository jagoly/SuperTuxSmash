#include <sqee/debug/Logging.hpp>
#include <sqee/misc/StringCast.hpp>

#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

#include <game/Misc.hpp>

#include <game/fighters/Sara/Fighter.hpp>

namespace maths = sq::maths;
using namespace sts::fighters;
using Context = sq::Context;

//============================================================================//

Sara_Fighter::Sara_Fighter() : Fighter("Sara") {}

Sara_Fighter::~Sara_Fighter() = default;

//============================================================================//

void Sara_Fighter::setup()
{
    SQASSERT(mController != nullptr, "");
    SQASSERT(mRenderer != nullptr, "");

    //========================================================//

    misc::load_actions_from_json(*this);

    //========================================================//

    ARMA_Sara.load_bones_from_file("fighters/Sara/armature.txt");
    ARMA_Sara.mRestPose = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/Rest.txt");

    POSE_Rest = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/Rest.txt");
    POSE_Stand = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/Stand.txt");
    POSE_Jump = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/Jump.txt");

    ANIM_Walk[0] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkA.txt");
    ANIM_Walk[1] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkB.txt");
    ANIM_Walk[2] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkC.txt");
    ANIM_Walk[3] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkD.txt");
    ANIM_Walk[4] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkE.txt");
    ANIM_Walk[5] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkF.txt");
    ANIM_Walk[6] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkG.txt");
    ANIM_Walk[7] = ARMA_Sara.load_pose_from_file("fighters/Sara/poses/walk/WalkH.txt");

    mPosePrevious = mPoseCurrent = POSE_Rest;

    ARMA_Sara.refresh_matrices();

    UBO_Sara.reserve("bones", 960u);
    UBO_Sara.create_and_allocate();

    //========================================================//

    MESH_Sara.load_from_file("fighters/Sara/mesh");

    //========================================================//

    const auto setup_texture = [](auto& texture, uint size, auto path)
    {
        texture.set_filter_mode(true);
        texture.set_mipmaps_mode(true);
        texture.allocate_storage(Vec2U(size));
        texture.load_file(path);
        texture.generate_auto_mipmaps();
    };

    setup_texture(TX_Main_diff, 2048u, "fighters/Sara/textures/Main_diff");
    setup_texture(TX_Main_spec, 2048u, "fighters/Sara/textures/Main_spec");

    setup_texture(TX_Hair_diff, 512u, "fighters/Sara/textures/Hair_diff");
    setup_texture(TX_Hair_norm, 512u, "fighters/Sara/textures/Hair_norm");
    setup_texture(TX_Hair_mask, 512u, "fighters/Sara/textures/Hair_mask");

    TX_Main_spec.set_swizzle_mode('R', 'R', 'R', '1');
    TX_Hair_mask.set_swizzle_mode('0', '0', '0', 'R');

    //========================================================//

    VS_Sara.add_uniform("u_model_mat"); // Mat4F
    VS_Sara.add_uniform("u_normal_mat"); // Mat3F
    FS_Hair.add_uniform("u_specular"); // Vec3F

    mRenderer->shaders.preprocs(VS_Sara, "fighters/Sara/Sara_vs");
    mRenderer->shaders.preprocs(FS_Main, "fighters/Sara/Main_fs");
    mRenderer->shaders.preprocs(FS_Hair, "fighters/Sara/Hair_fs");
}

//============================================================================//

void Sara_Fighter::tick()
{
    this->impl_tick_base();

    mPosePrevious = mPoseCurrent;

    switch (state.move) {

    case State::Move::None:
    {
        if (mAnimationSwitch == true)
        {
            mPoseCurrent = ARMA_Sara.blend_poses(mPoseCurrent, POSE_Stand, 0.5f);
            mAnimationSwitch = false;
        }
        else mPoseCurrent = POSE_Stand;

        mAnimationProgress = 0.f;
        mAnimationIndex = 0u;

        break;
    }

    case State::Move::Walking:
    {
        // todo: work out exact walk animation speed
        const float velocity = std::abs(mVelocity.x) / 10.f;

        if ((mAnimationProgress += velocity) >= 1.f)
        {
            mAnimationProgress = 0.f;
            if (++mAnimationIndex == 8u)
                mAnimationIndex = 0u;
        }

        const auto& current = ANIM_Walk[mAnimationIndex];
        const auto& next = ANIM_Walk[mAnimationIndex < 7u ? mAnimationIndex + 1u : 0u];
        mPoseCurrent = ARMA_Sara.blend_poses(current, next, mAnimationProgress);

        mAnimationSwitch = true;

        break;
    }

    case State::Move::Dashing:
    {
        // todo: work out exact dash animation speed
        const float velocity = std::abs(mVelocity.x) / 10.f;

        if ((mAnimationProgress += velocity) >= 1.f)
        {
            mAnimationProgress = 0.f;
            if (++mAnimationIndex == 8u)
                mAnimationIndex = 0u;
        }

        const auto& current = ANIM_Walk[mAnimationIndex];
        const auto& next = ANIM_Walk[mAnimationIndex < 7u ? mAnimationIndex + 1u : 0u];
        mPoseCurrent = ARMA_Sara.blend_poses(current, next, mAnimationProgress);

        mAnimationSwitch = true;

        break;
    }

    case State::Move::Jumping:
    {
        mPoseCurrent = POSE_Jump;

        mAnimationSwitch = true;

        mAnimationProgress = 0.f;
        mAnimationIndex = 0u;

        break;
    }

    } // switch (state.move)
}

//============================================================================//

void Sara_Fighter::render()
{
    static auto& context = Context::get();

    const auto& progress = mRenderer->progress;
    const auto& camera = mRenderer->camera;

    //========================================================//

    const auto blendPose = ARMA_Sara.blend_poses(mPosePrevious, mPoseCurrent, progress);

    const auto data = ARMA_Sara.pose_to_ubo_data(blendPose);
    UBO_Sara.update(0u, data.size() * 12u, data.data());

    //========================================================//

    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Replace);

    Vec3F scale; Vec3F tint; float rotation = 0.f;

    //========================================================//

    if (state.action == State::Action::None)
    {
        scale = Vec3F(1.f, 1.f, 1.f);
    }

    if (state.action == State::Action::Neutral)
    {
        scale = Vec3F(1.f, 1.f, 1.5f);
    }

    //========================================================//

    if (state.move == State::Move::None)
    {
        tint = Vec3F(1.f, 0.f, 0.f);
    }

    if (state.move == State::Move::Walking)
    {
        tint = Vec3F(0.f, 1.f, 0.f);
    }

    if (state.move == State::Move::Dashing)
    {
        tint = Vec3F(0.3f, 0.3f, 0.6f);
    }

    if (state.move == State::Move::Jumping)
    {
        tint = Vec3F(0.f, 0.f, 1.f);
    }

    if (state.move == State::Move::Falling)
    {
        tint = Vec3F(0.1f, 0.1f, 2.f);
    }

    //========================================================//

    if (state.direction == State::Direction::Left)
    {
        rotation = -0.25f;
    }

    if (state.direction == State::Direction::Right)
    {
        rotation = +0.25f;
    }

    //========================================================//

    Vec2F position = maths::mix(previous.position, current.position, progress);
    const Mat4F modelMatrix = maths::transform(Vec3F(position.x, 0.f, position.y), QuatF(0.f, 0.f, rotation), scale);
    const Mat3F normalMatrix = maths::normal_matrix(camera.viewMatrix * modelMatrix);

    //========================================================//

    context.use_Shader_Vert(VS_Sara);

    VS_Sara.update("u_model_mat", modelMatrix);
    VS_Sara.update("u_normal_mat", normalMatrix);

    context.bind_VertexArray(MESH_Sara.get_vao());
    context.bind_UniformBuffer(UBO_Sara, 2u);

    //========================================================//

    context.bind_Texture(TX_Main_diff, 0u);
    context.bind_Texture(TX_Main_spec, 2u);

    context.use_Shader_Frag(FS_Main);

    MESH_Sara.draw_partial(0u);

    //========================================================//

    context.bind_Texture(TX_Hair_diff, 0u);
    context.bind_Texture(TX_Hair_norm, 1u);
    context.bind_Texture(TX_Hair_mask, 3u);

    context.use_Shader_Frag(FS_Hair);

    FS_Hair.update("u_specular", Vec3F(0.5f, 0.4f, 0.1f));

    MESH_Sara.draw_partial(1u);
}
