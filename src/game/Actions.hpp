#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include "game/forward.hpp"

namespace sts {

//============================================================================//

class Action : sq::NonCopyable
{
public: //====================================================//

    enum class Type
    {
        None = -1,
        Neutral_First,
        Tilt_Down,
        Tilt_Forward,
        Tilt_Up,
        Air_Back,
        Air_Down,
        Air_Forward,
        Air_Neutral,
        Air_Up,
        Dash_Attack,
    };

    //--------------------------------------------------------//

    Action(FightSystem& system, Fighter& fighter)
        : system(system), fighter(fighter) {}

    virtual ~Action();

protected: //=================================================//

    /// Jump action to the specified frame.
    void jump_to_frame(uint frame);

    //--------------------------------------------------------//

    /// Do stuff when action first starts.
    virtual void on_start() {}

    /// Return true when the action is finished.
    virtual bool on_tick(uint frame) = 0;

    /// Do stuff when a blob collides with a fighter.
    virtual void on_collide(HitBlob* blob, Fighter& other, Vec3F point) = 0;

    /// Do stuff when finished or cancelled.
    virtual void on_finish() = 0;

    //--------------------------------------------------------//

    FightSystem& system;
    Fighter& fighter;

    std::array<HitBlob*, 13> blobs {};

private: //===================================================//

    uint mCurrentFrame = 0u;

    char mPadding[4] {};

    //--------------------------------------------------------//

    bool impl_do_tick();

    //--------------------------------------------------------//

    friend class FightSystem;
    friend class Actions;
};

//============================================================================//

class Actions final : public sq::NonCopyable
{
public: //====================================================//

    void switch_active(Action::Type type)
    {
        if (mActiveAction != nullptr) mActiveAction->on_finish();

        if (type == Action::Type::None)          mActiveAction = nullptr;
        if (type == Action::Type::Neutral_First) mActiveAction = neutral_first.get();
        if (type == Action::Type::Tilt_Down)     mActiveAction = tilt_down.get();
        if (type == Action::Type::Tilt_Forward)  mActiveAction = tilt_forward.get();
        if (type == Action::Type::Tilt_Up)       mActiveAction = tilt_up.get();
        if (type == Action::Type::Air_Back)      mActiveAction = air_back.get();
        if (type == Action::Type::Air_Down)      mActiveAction = air_down.get();
        if (type == Action::Type::Air_Forward)   mActiveAction = air_forward.get();
        if (type == Action::Type::Air_Neutral)   mActiveAction = air_neutral.get();
        if (type == Action::Type::Air_Up)        mActiveAction = air_up.get();
        if (type == Action::Type::Dash_Attack)   mActiveAction = dash_attack.get();

        mActiveType = type;

        mActiveAction->mCurrentFrame = 0u;
        mActiveAction->on_start();
    }

    //--------------------------------------------------------//

    /// Get the type of the active action.
    auto active_type() const { return mActiveType; }

    //--------------------------------------------------------//

    /// Tick the active action if not none.
    void tick_active_action();

    //--------------------------------------------------------//

    unique_ptr<Action> neutral_first;

    unique_ptr<Action> tilt_down;
    unique_ptr<Action> tilt_forward;
    unique_ptr<Action> tilt_up;

    unique_ptr<Action> air_back;
    unique_ptr<Action> air_down;
    unique_ptr<Action> air_forward;
    unique_ptr<Action> air_neutral;
    unique_ptr<Action> air_up;

    unique_ptr<Action> dash_attack;

private: //===================================================//

    Action::Type mActiveType = Action::Type::None;
    Action* mActiveAction = nullptr;
};

//============================================================================//

class DumbAction final : public Action
{
public: //====================================================//

    DumbAction(FightSystem& system, Fighter& fighter, string message)
        : Action(system, fighter), message(message) {}

private: //===================================================//

    void on_start() override;

    bool on_tick(uint frame) override;

    void on_collide(HitBlob*, Fighter&, Vec3F) override;

    void on_finish() override;

    //--------------------------------------------------------//

    const string message;
};

//============================================================================//

} // namespace sts
