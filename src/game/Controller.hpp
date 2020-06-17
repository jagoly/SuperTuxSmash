#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/app/InputDevices.hpp>
#include <sqee/app/Event.hpp>

#include "main/Globals.hpp"

//============================================================================//

namespace sts {

class Controller final : sq::NonCopyable
{
public: //====================================================//

    Controller(const Globals& globals, const sq::InputDevices& devices, String configPath);

    //--------------------------------------------------------//

    struct Input
    {
        bool press_attack = false;
        bool press_jump = false;

        bool hold_attack = false;
        bool hold_jump = false;

        bool press_shield = false;
        bool hold_shield = false;

        Vec2F float_axis {};

        struct { int8_t x=0, y=0; } int_axis;
        struct { int8_t x=0, y=0; } mash_axis;
        struct { int8_t x=0, y=0; } mod_axis;
        struct { int8_t x=0, y=0; } norm_axis;
    };

    //--------------------------------------------------------//

    /// Handle a system input event.
    void handle_event(sq::Event event);

    /// Refresh and access input data.
    Input get_input();

    //--------------------------------------------------------//

    const Globals& globals;
    const sq::InputDevices& devices;

private: //===================================================//

    struct {

        int gamepad_port = -1;

        sq::Gamepad_Stick stick_move = sq::Gamepad_Stick::Unknown;

        sq::Gamepad_Button button_attack = sq::Gamepad_Button::Unknown;
        sq::Gamepad_Button button_jump   = sq::Gamepad_Button::Unknown;
        sq::Gamepad_Button button_shield = sq::Gamepad_Button::Unknown;

        sq::Keyboard_Key key_left  = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_up    = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_right = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_down  = sq::Keyboard_Key::Unknown;

        sq::Keyboard_Key key_attack = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_jump   = sq::Keyboard_Key::Unknown;
        sq::Keyboard_Key key_shield = sq::Keyboard_Key::Unknown;

    } config;

    //--------------------------------------------------------//

    Vec2F mPrevAxisMove;

    uint mTimeSinceZeroX = 0u;
    uint mTimeSinceZeroY = 0u;

    bool mDoneMashX = false;
    bool mDoneMashY = false;

    Input mInput;
};

} // namespace sts
