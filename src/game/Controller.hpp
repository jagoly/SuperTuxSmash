#pragma once

#include <sqee/builtins.hpp>

#include <sqee/app/InputDevices.hpp>
#include <sqee/app/Event.hpp>

//============================================================================//

#ifndef SQEE_MSVC

#define DISABLE_FLOAT_EQUALITY_WARNING \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wfloat-equal\"")

#define ENABLE_FLOAT_EQUALITY_WARNING \
_Pragma("GCC diagnostic pop")

#else

#define DISABLE_FLOAT_EQUALITY_WARNING
#define ENABLE_FLOAT_EQUALITY_WARNING

#endif

//============================================================================//

namespace sts {

class Controller final : sq::NonCopyable
{
public: //====================================================//

    Controller(uint8_t index, const sq::InputDevices& devices, string configPath);

    //--------------------------------------------------------//

    struct Input
    {
        bool press_attack = false;
        bool press_jump = false;

        bool hold_attack = false;
        bool hold_jump = false;

        int8_t mod_axis_x = 0;
        int8_t mod_axis_y = 0;

        int8_t mash_axis_x = 0;
        int8_t mash_axis_y = 0;

        Vec2F axis_move {};
    };

    //--------------------------------------------------------//

    /// Handle a system input event.
    void handle_event(sq::Event event);

    /// Access controller state.
    Input get_input();

    //--------------------------------------------------------//

    const uint8_t index;

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

    Vec2F mPrevAxisMove;

    uint mTimeSinceZeroX = 0u;
    uint mTimeSinceZeroY = 0u;

    bool mDoneMashX = false;
    bool mDoneMashY = false;

    Input mInput;

    //--------------------------------------------------------//

    const sq::InputDevices& mDevices;
};

} // namespace sts
