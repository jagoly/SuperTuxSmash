#pragma once

#include <sqee/builtins.hpp>

#include "game/Entity.hpp"
#include "game/Controller.hpp"
#include "game/Actions.hpp"

#include "game/forward.hpp"

// todo: should fighter really inherit from Entity?

//============================================================================//

namespace sts {

class Fighter : public Entity
{
public: //====================================================//

    struct State {

        enum class Move { None, Walking, Dashing, Jumping, Falling } move;
        enum class Direction { Left = -1, Right = +1 } direction;

    } state;

    //--------------------------------------------------------//

    Fighter(uint8_t index, FightSystem& system, Controller& controller, string name);

    virtual ~Fighter() override;

    //--------------------------------------------------------//

    virtual void tick() override = 0;

    //--------------------------------------------------------//

    float get_direction_factor() const
    {
        return float(state.direction);
    }

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

    //--------------------------------------------------------//

    /// Index of the fighter.
    const uint8_t index;

protected: //=================================================//

    Vec2F mVelocity = { 0.f, 0.f };

    bool mAttackHeld = false;
    bool mJumpHeld = false;

    Controller& mController;

    unique_ptr<Actions> mActions;

    std::vector<HitBlob*> mHurtBlobs;

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
