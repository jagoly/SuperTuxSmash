#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include <game/Actions.hpp>
#include <game/Controller.hpp>

namespace sts {

//============================================================================//

class Game; // Forward Declaration

//============================================================================//

class Fighter : sq::NonCopyable
{
public:

    //========================================================//

    struct State {

        enum class Move { None, Walking, Dashing, Jumping, Falling } move;
        enum class Direction { Left, Right } direction;

    } state;

    //========================================================//

    Fighter(string name, Game& game);

    virtual ~Fighter() = default;

    //========================================================//

    const string name;
    Game& game;

    //========================================================//

    virtual void setup() = 0;

    virtual void tick() = 0;

    virtual void integrate() = 0;

    virtual void render_depth() = 0;

    virtual void render_main() = 0;

    //========================================================//

    struct Frame
    {
        Vec2F velocity;
        Vec2F position;
    };

    Frame previous, current;

    //========================================================//

    unique_ptr<Controller> controller;
    unique_ptr<Actions> actions;

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

    void impl_input_movement(Controller::Input input);
    void impl_input_actions(Controller::Input input);

    void impl_update_fighter();

    //========================================================//

    void impl_tick_base();

    void impl_validate_stats();

    //========================================================//

    bool mAttackHeld = false;
    bool mJumpHeld = false;
};

//============================================================================//

} // namespace sts
