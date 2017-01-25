#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

// Forward Declaration
namespace sf { class Event; }

namespace sts {

//============================================================================//

class Controller final : sq::NonCopyable
{
public:

    //========================================================//

    struct Input
    {
        bool press_attack = false;
        bool press_jump = false;

        bool hold_attack = false;
        bool hold_jump = false;

        bool activate_dash = false;

        Vec2F axis_move {};
    };

    //========================================================//

    void load_config(const string& path);

    bool handle_event(sf::Event event);

    //========================================================//

    Input get_input();

private:

    //========================================================//

    struct {

        int joystick_id    = -1;

        int button_attack  = -1;
        int button_jump    = -1;

        int button_left    = -1;
        int button_right   = -1;
        int button_down    = -1;
        int button_up      = -1;

        int axis_move_x    = -1;
        int axis_move_y    = -1;

    } config;

    //========================================================//

    uint mTimeSinceNotLeft = 0u;
    uint mTimeSinceNotRight = 0u;

    Vec2F mPrevAxisMove;

    Input mInput;
};

//============================================================================//

} // namespace sts
