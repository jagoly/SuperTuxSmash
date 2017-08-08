#pragma once

#include <sqee/builtins.hpp>

#include <sqee/render/Armature.hpp>

#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/Actions.hpp"

//============================================================================//

namespace sts {

class Fighter : sq::NonCopyable
{
public: //====================================================//

    struct State {

        enum class Move { None, Walk, Dash, Air, Knock } move;
        enum class Direction { Left = -1, Right = +1 } direction;

        bool helpless = false;
        bool stunned = false;

    } state;

    //--------------------------------------------------------//

    Fighter(uint8_t index, FightWorld& world, Controller& controller, string path);

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    struct {

        float walk_speed      = 1.f; // 5m/s
        float dash_speed      = 1.f; // 8m/s
        float air_speed       = 1.f; // 5m/s
        float land_traction   = 1.f; // 0.5
        float air_traction    = 1.f; // 0.2
        float hop_height      = 1.f; // 2m
        float jump_height     = 1.f; // 3m
        float fall_speed      = 1.f; // 15m/s

    } stats;

    //--------------------------------------------------------//

    /// Index of the fighter.
    const uint8_t index;

    //--------------------------------------------------------//

    /// Update the armature pose.
    void update_pose(const sq::Armature::Pose& pose);

    /// Update the pose from a continuous animation.
    void update_pose(const sq::Armature::Animation& anim, float time);

    /// Play an animation with discrete timing.
    void play_animation(const sq::Armature::Animation& anim);

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    //--------------------------------------------------------//

    /// Access the active animation, or nullptr.
    const sq::Armature::Animation* get_animation() const { return mAnimation; }

    //--------------------------------------------------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const std::vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    std::vector<Mat34F> interpolate_bone_matrices(float blend) const;

protected: //=================================================//

    unique_ptr<Actions> mActions;

    sq::Armature mArmature;

    std::vector<HurtBlob*> mHurtBlobs;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;

    Controller& mController;

    bool mAttackHeld = false;
    bool mJumpHeld = false;

    Vec2F mVelocity = { 0.f, 0.f };

    //--------------------------------------------------------//

    void base_tick_fighter();

    void base_tick_animation();

private: //===================================================//

    struct InterpolationData
    {
        Vec2F position;
        sq::Armature::Pose pose;
    }
    previous, current;

    //--------------------------------------------------------//

    const sq::Armature::Animation* mAnimation = nullptr;

    uint mAnimationTime = 0u;

    std::vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

    //--------------------------------------------------------//

    void impl_initialise_armature(const string& path);

    void impl_initialise_hurt_blobs(const string& path);

    void impl_initialise_stats(const string& path);

    //--------------------------------------------------------//

    void impl_input_movement(Controller::Input input);
    void impl_input_actions(Controller::Input input);

    void impl_update_physics();
};

//============================================================================//

} // namespace sts
