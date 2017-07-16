#pragma once

#include <functional>

#include <sqee/builtins.hpp>
#include <sqee/assert.hpp>

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

    virtual ~Action();

protected: //=================================================//

    struct FrameMethod
    {
        FrameMethod(uint frame) : frame(frame) {}
        const uint frame; std::function<void()> func;
    };

    //--------------------------------------------------------//

    /// Add a method for the specified frame.
    std::function<void()>& add_frame_method(uint frame);

    /// Jump action to the specified frame.
    void jump_to_frame(uint frame);

    /// Check what frame the action is currently on.
    uint get_current_frame() const { return mCurrentFrame; }

    //--------------------------------------------------------//

    /// Do stuff when action first starts.
    virtual void on_start() {}

    /// Do stuff when action finishes normally.
    virtual void on_finish() {}

    /// Do stuff when action is cancelled early.
    virtual void on_cancel() {}

    /// Return true to finish the action.
    virtual bool on_tick() = 0;

private: //===================================================//

    std::vector<FrameMethod> mMethodVec;
    std::vector<FrameMethod>::iterator mMethodIter;

    uint mCurrentFrame = 0u;

    //--------------------------------------------------------//

    void impl_start();

    void impl_cancel();

    bool impl_tick();

    //--------------------------------------------------------//

    friend class Actions;
};

//============================================================================//

class Actions final : public sq::NonCopyable
{
public: //====================================================//

    void switch_active(Action::Type type)
    {
        if (mActiveAction != nullptr) mActiveAction->impl_cancel();

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

        mActiveAction->impl_start();
    }

    //--------------------------------------------------------//

    /// Get the type of the active action.
    Action::Type active_type() const { return mActiveType; }

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

    DumbAction(string message) : mMessage(message) {}

private: //===================================================//

    void on_start() override;
    void on_finish() override;
    void on_cancel() override;
    bool on_tick() override;

    //--------------------------------------------------------//

    const string mMessage;
};

//============================================================================//

} // namespace sts
