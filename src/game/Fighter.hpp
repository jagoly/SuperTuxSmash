#pragma once

#include <sqee/builtins.hpp>

#include "game/FightWorld.hpp"
#include "game/Actions.hpp"

namespace sts {

//====== Forward Declarations ================================================//

class PrivateFighter;
class Controller;

//============================================================================//

class Fighter : sq::NonCopyable
{
public: //====================================================//

    enum class State
    {
        Neutral, Walking, Dashing, Brake, Crouch,
        Charge, Attack, Landing, PreJump, Jumping,
        Falling, AirAttack, Knocked, Stunned
    };

    enum class Facing
    {
        Left = -1, Right = +1
    };

    //--------------------------------------------------------//

    struct Stats
    {
        float walk_speed      = 1.f;
        float dash_speed      = 1.f;
        float air_speed       = 1.f;
        float traction        = 1.f;
        float air_mobility    = 1.f;
        float air_friction    = 1.f;
        float hop_height      = 1.f;
        float jump_height     = 1.f;
        float air_hop_height  = 1.f;
        float gravity         = 1.f;
        float fall_speed      = 1.f;
    };

    //--------------------------------------------------------//

    Fighter(uint8_t index, FightWorld& world, string_view name);

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    State state = State::Neutral;

    Facing facing = Facing::Right;

    Stats stats;

    //--------------------------------------------------------//

    string_view get_name() const { return mName; }

    //--------------------------------------------------------//

    // temporary, for debug

    void set_controller(Controller* controller);
    Controller* get_controller();

    //--------------------------------------------------------//

    Actions& get_actions() { return *mActions; }

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- access data needed for world updates ----------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const std::vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    /// Get current velocity.
    const Vec2F& get_velocity() const { return mVelocity; }

    /// Get world space physics diamond.
    const PhysicsDiamond& get_diamond() const { return mWorldDiamond; }

    //-- compute data needed for rendering -------------------//

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    std::vector<Mat34F> interpolate_bone_matrices(float blend) const;

    //--------------------------------------------------------//

    // todo: make better interface for actions to use

    Vec2F mVelocity = { 0.f, 0.f };

protected: //=================================================//

    unique_ptr<Actions> mActions;

    std::vector<HurtBlob*> mHurtBlobs;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;

    PhysicsDiamond mLocalDiamond, mWorldDiamond;

    //--------------------------------------------------------//

    void base_tick_fighter();

    void base_tick_animation();

    //--------------------------------------------------------//

    string_view mName;

private: //===================================================//

    std::vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

    //--------------------------------------------------------//

    void impl_show_fighter_widget();

    //--------------------------------------------------------//

    friend class GameScene;

    friend class PrivateFighter;
    unique_ptr<PrivateFighter> impl;
};

//============================================================================//

#define ETSC SQEE_ENUM_TO_STRING_CASE

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(Fighter::State)
ETSC(Neutral) ETSC(Walking) ETSC(Dashing) ETSC(Brake) ETSC(Crouch)
ETSC(Charge) ETSC(Attack) ETSC(Landing) ETSC(PreJump) ETSC(Jumping)
ETSC(Falling) ETSC(AirAttack) ETSC(Knocked) ETSC(Stunned)
SQEE_ENUM_TO_STRING_BLOCK_END

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(Fighter::Facing)
ETSC(Left) ETSC(Right)
SQEE_ENUM_TO_STRING_BLOCK_END

#undef ETSC

//============================================================================//

} // namespace sts
