#pragma once

#include <sqee/builtins.hpp>

#include <sqee/app/InputDevices.hpp>
#include <sqee/app/Event.hpp>

//============================================================================//

namespace sts {

class Controller final : sq::NonCopyable
{
public: //====================================================//

    Controller(const sq::InputDevices& devices, string configPath);

    //--------------------------------------------------------//

    struct Input
    {
        bool press_attack = false;
        bool press_jump = false;

        bool hold_attack = false;
        bool hold_jump = false;

        bool activate_dash = false;

        Vec2F axis_move {};
    };

    //--------------------------------------------------------//

    /// Handle a system input event.
    void handle_event(sq::Event event);

    /// Access controller state.
    Input get_input();

private: //===================================================//

    struct {

        int gamepad_port = -1;

        sq::Gamepad_Stick stick_move = sq::Gamepad_Stick::Unknown;

        sq::Gamepad_Button button_attack = sq::Gamepad_Button::Unknown;
        sq::Gamepad_Button button_jump   = sq::Gamepad_Button::Unknown;

        sq::Keyboard_Key key_left  = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_up    = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_right = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_down  = sq::Keyboard_Key::Unknown;

        sq::Keyboard_Key key_attack = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_jump   = sq::Keyboard_Key::Unknown;

    } config;

    //--------------------------------------------------------//

    uint mTimeSinceNotLeft = 0u;
    uint mTimeSinceNotRight = 0u;

    Vec2F mPrevAxisMove;

    Input mInput;

    //--------------------------------------------------------//

    const sq::InputDevices& mDevices;
};

} // namespace sts
