#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/render/Armature.hpp>

#include "game/FightWorld.hpp"
#include "game/Actions.hpp"

namespace sts {

//============================================================================//

class Fighter : sq::NonCopyable
{
public: //====================================================//

    enum class Command
    {
        Shield,
        Jump,
        MashDown,
        MashUp,
        MashLeft,
        MashRight,
        TurnLeft,
        TurnRight,
        SmashDown,
        SmashUp,
        SmashLeft,
        SmashRight,
        AttackDown,
        AttackUp,
        AttackLeft,
        AttackRight,
        AttackNeutral,
    };

    enum class State
    {
        Neutral,
        Walking,
        Dashing,
        Brake,
        Crouch,
        Charge,
        Action,
        AirAction,
        Landing,
        PreJump,
        Jumping,
        Helpless,
        Knocked,
        Stunned,
        Shield,
        LedgeHang,
        LedgeClimb,
        EditorPreview,
    };

    enum class AnimMode
    {
        Standard,    ///< non-looping, without root motion
        Looping,     ///< looping, without root motion
        WalkCycle,   ///< looping, update using velocity and anim_walk_stride
        DashCycle,   ///< looping, update using velocity and anim_dash_stride
        ApplyMotion, ///< non-looping, extract root motion to object
    };

    struct Animation
    {
        sq::Armature::Animation anim;
        AnimMode mode;
        String key;
    };

    //--------------------------------------------------------//

    struct Stats
    {
        float walk_speed        = 1.f;
        float dash_speed        = 1.f;
        float air_speed         = 1.f;
        float traction          = 1.f;
        float air_mobility      = 1.f;
        float air_friction      = 1.f;
        float hop_height        = 1.f;
        float jump_height       = 1.f;
        float air_hop_height    = 1.f;
        float ledge_jump_height = 1.f;
        float gravity           = 1.f;
        float fall_speed        = 1.f;
        float weight            = 1.f;

        uint extra_jumps = 2u;

        uint dash_start_time  = 12u;
        uint dash_brake_time  = 12u;
        uint dash_turn_time   = 14u;
        uint ledge_climb_time = 34u;

        uint land_heavy_min_time = 4u;

        float anim_walk_stride = 2.f;
        float anim_dash_stride = 3.f;
    };

    //--------------------------------------------------------//

    struct Status
    {
        bool intangible = false;
        float damage = 0.f;
        float shield = 0.f;
        struct Ledge* ledge = nullptr;
    };

    //--------------------------------------------------------//

    virtual ~Fighter();

    //--------------------------------------------------------//

    virtual void tick() = 0;

    //--------------------------------------------------------//

    const uint8_t index;

    const FighterEnum type;

    Stats stats;

    Status status;

    //--------------------------------------------------------//

    // all of the data edited by the editor should be here for now
    // in the future, may make them private and add getters

    Array<UniquePtr<Action>, sq::enum_count_v<ActionType>> actions;

    sq::PoolMap<TinyString, HurtBlob> hurtBlobs;

    LocalDiamond diamond;

    //--------------------------------------------------------//

    struct {

        State state = State::Neutral;
        int8_t facing = +1;

        Action* action = nullptr;

    } current, previous;

    //--------------------------------------------------------//

    const sq::Armature& get_armature() const;

    //--------------------------------------------------------//

    void set_controller(Controller* controller);
    Controller* get_controller();

    //--------------------------------------------------------//

    Action* get_action(ActionType type);

    //--------------------------------------------------------//

    /// Called when hit by a basic attack.
    void apply_hit_basic(const HitBlob& hit);

    /// Called when passing the stage boundary.
    void pass_boundary();

    //-- access data needed for world updates ----------------//

    /// Get current model matrix.
    const Mat4F& get_model_matrix() const { return mModelMatrix; }

    /// Get current armature pose matrices.
    const Vector<Mat34F>& get_bone_matrices() const { return mBoneMatrices; }

    Vec2F get_position() const; ///< Get current position.

    Vec2F get_velocity() const; ///< Get current velocity.

    //-- compute data needed for rendering -------------------//

    /// Compute interpolated model matrix.
    Mat4F interpolate_model_matrix(float blend) const;

    /// Compute interpolated armature pose matrices.
    void interpolate_bone_matrices(float blend, Mat34F* out, size_t len) const;

    //-- lua properties and methods --------------------------//

    bool lua_get_intangible() const { return status.intangible; }
    void lua_set_intangible(bool value) { status.intangible = value; }

    float lua_get_velocity_x() const { return mVelocity.x; }
    void lua_set_velocity_x(float value) { mVelocity.x = value; }

    float lua_get_velocity_y() const { return mVelocity.y; }
    void lua_set_velocity_y(float value) { mVelocity.y = value; }

    void lua_func_change_facing();

    //-- debug stuff -----------------------------------------//

    Vector<Mat4F> debug_get_skeleton_mats() const;

protected: //=================================================//

    Fighter(uint8_t index, FightWorld& world, FighterEnum type);

    //--------------------------------------------------------//

    FightWorld& mFightWorld;

    Vec2F mVelocity = { 0.f, 0.f };
    Vec2F mTranslate = { 0.f, 0.f };

    //--------------------------------------------------------//

    void base_tick_fighter();

    //--------------------------------------------------------//

    Array<Vector<Command>, 8u> mCommands;

    bool consume_command(Command cmd);

    bool consume_command_oldest(InitList<Command> cmds);

    bool consume_command_facing(Command leftCmd, Command rightCmd);

    bool consume_command_oldest_facing(InitList<Command> leftCmds, InitList<Command> rightCmds);

private: //===================================================//

    Vector<Mat34F> mBoneMatrices;

    Mat4F mModelMatrix;

    //--------------------------------------------------------//

    friend class PrivateFighter;
    UniquePtr<PrivateFighter> impl;

public: //== debug and editor interfaces =====================//

    void debug_reload_actions();

    PrivateFighter* editor_get_private() { return impl.get(); }

    friend struct DebugGui;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::Fighter::State,
                 Neutral,
                 Walking,
                 Dashing,
                 Brake,
                 Crouch,
                 Charge,
                 Action,
                 AirAction,
                 Landing,
                 PreJump,
                 Jumping,
                 Helpless,
                 Knocked,
                 Stunned,
                 Shield,
                 LedgeHang,
                 LedgeClimb,
                 EditorPreview)

SQEE_ENUM_HELPER(sts::Fighter::Command,
                 Shield,
                 Jump,
                 MashDown,
                 MashUp,
                 MashLeft,
                 MashRight,
                 TurnLeft,
                 TurnRight,
                 SmashDown,
                 SmashUp,
                 SmashLeft,
                 SmashRight,
                 AttackDown,
                 AttackUp,
                 AttackLeft,
                 AttackRight,
                 AttackNeutral)
