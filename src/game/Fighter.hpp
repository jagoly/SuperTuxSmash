#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include <game/Actions.hpp>
#include <game/Controller.hpp>
#include <game/Renderer.hpp>

namespace sts {

//============================================================================//

class Fighter : sq::NonCopyable
{
public:

    //========================================================//

    struct State {

        enum class Action { None, Neutral, Tilt, Air, Dash } action;
        enum class Move { None, Walking, Dashing, Jumping, Falling } move;
        enum class Direction { Left, Right } direction;

    } state;

    //========================================================//

    Fighter(string name);

    virtual ~Fighter() = default;

    //========================================================//

    virtual void setup() = 0;

    virtual void tick() = 0;

    virtual void render() = 0;

    //========================================================//

    const string mName;

    Controller* mController = nullptr;
    Renderer* mRenderer = nullptr;

    //========================================================//

    struct TickState
    {
        Vec2F position;
        Vec3F colour;
    };

    TickState previous, current;

    //========================================================//

    struct {

        unique_ptr<Action> neutral_first;
        unique_ptr<Action> neutral_second;
        unique_ptr<Action> neutral_third;

        unique_ptr<Action> tilt_down;
        unique_ptr<Action> tilt_forward;
        unique_ptr<Action> tilt_up;

        //unique_ptr<Action> air_neutral;
        //unique_ptr<Action> air_back;
        //unique_ptr<Action> air_forward;
        //unique_ptr<Action> air_down;
        //unique_ptr<Action> air_up;

        //unique_ptr<Action> dashing;

        Action* active = nullptr;

    } actions;

    //========================================================//

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

protected:

    //========================================================//

    void impl_update_before();

    void impl_input_actions(Controller::Input input);
    void impl_input_movement(Controller::Input input);

    void impl_update_after();

    //========================================================//

    void impl_tick_base();

    void impl_validate_stats();

    //========================================================//

    Vec2F mVelocity = { 0.f, 0.f };

    bool mJumpHeld = false;
};

//============================================================================//

} // namespace sts
