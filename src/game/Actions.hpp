#pragma once

#include <functional>

#include <sqee/builtins.hpp>

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

    /// Do stuff when action finishes normally.
    virtual void on_finish() {}

    /// Do stuff when action is cancelled early.
    virtual void on_cancel() {}

    //--------------------------------------------------------//

    /// Return true to finish action.
    virtual bool on_tick(uint frame) = 0;

    /// Do stuff upon collision with another hit blob.
    virtual void on_collide(HitBlob* blob, HitBlob* other) = 0;

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
        if (mActiveAction != nullptr) mActiveAction->on_cancel();

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
    void on_finish() override;
    void on_cancel() override;

    //--------------------------------------------------------//

    bool on_tick(uint frame) override;

    void on_collide(HitBlob*, HitBlob*) override;

    //--------------------------------------------------------//

    const string message;
};

//============================================================================//

} // namespace sts
