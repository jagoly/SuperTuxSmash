#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include <game/Attacks.hpp>
#include <game/Controller.hpp>

namespace sts {

//============================================================================//

class Stage; // Forward Declaration

//============================================================================//

class Fighter : sq::NonCopyable
{
public:

    //========================================================//

    struct State {
        enum class Attack { None, Neutral, Tilt, Air, Dash } attack;
        enum class Move { None, Walking, Dashing, Jumping, Falling } move;
        enum class Direction { Left, Right } direction;
    } state;

    //========================================================//

    unique_ptr<Attacks> attacks;

    //========================================================//

    Fighter(string name, Stage& stage);

    virtual ~Fighter();

    //========================================================//

    virtual void tick() = 0;

    //========================================================//

    struct TickState
    {
        Vec2F position;
        Vec3F colour;
    };

    TickState previous, current;

    //========================================================//

    Controller mController;

protected:

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

    //========================================================//

    struct {

        uint sinceAttackStart = 0u;

    } timers;

    //========================================================//

    void impl_update_before();

    void impl_input_attacks(Controller::Input input);
    void impl_input_movement(Controller::Input input);

    void impl_update_after();

    //========================================================//

    void impl_tick_base();

    void impl_validate_stats();

    //========================================================//

    Vec2F mVelocity = { 0.f, 0.f };

    bool mJumpHeld = false;

    const string mName;

    Stage& mStage;

};

//============================================================================//

} // namespace sts
