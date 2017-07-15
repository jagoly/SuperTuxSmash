#pragma once

#include <functional>

#include <sqee/builtins.hpp>
#include <sqee/assert.hpp>

namespace sts {

//============================================================================//

class Action : sq::NonCopyable
{
public: //====================================================//

    virtual ~Action();

    void start();

    void cancel();

    bool tick();

protected: //=================================================//

    struct FrameMethod
    {
        const uint frame;
        std::function<void()> func;
    };

    //--------------------------------------------------------//

    std::function<void()>& add_frame_method(uint frame)
    {
        SQASSERT(mMethodVec.empty() || frame > mMethodVec.back().frame, "");

        mMethodVec.push_back(FrameMethod{frame, {}});
        return mMethodVec.back().func;
    }

    //--------------------------------------------------------//

    uint get_current_frame() const
    {
        return mCurrentFrame;
    }

    //--------------------------------------------------------//

    virtual void on_start() {}

    virtual void on_finish() {}

    virtual void on_cancel() {}

    virtual bool on_tick() = 0;

private: //===================================================//

    std::vector<FrameMethod> mMethodVec;
    std::vector<FrameMethod>::iterator mMethodIter;

    uint mCurrentFrame = 0u;
};

//============================================================================//

class DumbAction final : public Action
{
public: //====================================================//

    DumbAction(string message) : mMessage(message) {}

protected: //=================================================//

    void on_start() override;
    void on_finish() override;
    void on_cancel() override;
    bool on_tick() override;

    //--------------------------------------------------------//

    const string mMessage;
};

//============================================================================//

class Actions final : public sq::NonCopyable
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

    void switch_active(Type type)
    {
        if (active.action != nullptr) active.action->cancel();

        if (type == Type::None)          active.action = nullptr;
        if (type == Type::Neutral_First) active.action = neutral_first.get();
        if (type == Type::Tilt_Down)     active.action = tilt_down.get();
        if (type == Type::Tilt_Forward)  active.action = tilt_forward.get();
        if (type == Type::Tilt_Up)       active.action = tilt_up.get();
        if (type == Type::Air_Back)      active.action = air_back.get();
        if (type == Type::Air_Down)      active.action = air_down.get();
        if (type == Type::Air_Forward)   active.action = air_forward.get();
        if (type == Type::Air_Neutral)   active.action = air_neutral.get();
        if (type == Type::Air_Up)        active.action = air_up.get();
        if (type == Type::Dash_Attack)   active.action = dash_attack.get();

        active.type = type;

        active.action->start();
    }

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

    //--------------------------------------------------------//

    struct {
        Type type = Type::None;
        Action* action = nullptr;
    } active;
};

//============================================================================//

} // namespace sts
