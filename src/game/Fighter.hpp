#pragma once

#include "game/Actions.hpp"
#include "game/Controller.hpp"
#include "game/Entity.hpp"

namespace sts {

//============================================================================//

class Fighter : public Entity
{
public: //====================================================//

    struct State {

        enum class Move { None, Walking, Dashing, Jumping, Falling } move;
        enum class Direction { Left, Right } direction;

    } state;

    //--------------------------------------------------------//

    Fighter(const string& name, Controller& controller);

    virtual ~Fighter() override = default;

    //--------------------------------------------------------//

    virtual void tick() override = 0;

    //--------------------------------------------------------//

    Controller& controller;

    unique_ptr<Actions> actions;

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

protected: //=================================================//

    Vec2F mVelocity = { 0.f, 0.f };

    bool mAttackHeld = false;
    bool mJumpHeld = false;

    //--------------------------------------------------------//

    void base_tick_fighter();

private: //===================================================//

    void impl_input_movement(Controller::Input input);
    void impl_input_actions(Controller::Input input);

    void impl_update_fighter();

    void impl_validate_stats();
};

//============================================================================//

} // namespace sts
