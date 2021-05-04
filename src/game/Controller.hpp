#pragma once

#include "setup.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/Gamepad.hpp>
#include <sqee/app/InputDevices.hpp>

namespace sts {

//============================================================================//

/// One frame of input from a Controller.
struct InputFrame final
{
    bool press_attack = false;
    bool press_jump = false;
    bool press_shield = false;

    bool hold_attack = false;
    bool hold_jump = false;
    bool hold_shield = false;

    Vec2F float_axis; ///< -1, -0.5, -0, +0, +0.5, +1

    maths::Vector2<int8_t> int_axis;  ///< in the range of -2 to +2
    maths::Vector2<int8_t> norm_axis; ///< just int_axis but normalised
    maths::Vector2<int8_t> mash_axis; ///< true for one frame if you quickly push all the way
    maths::Vector2<int8_t> mod_axis;  ///< same as mash_axis but lasts for a few frames
};

//============================================================================//

class Controller final : sq::NonCopyable
{
public: //====================================================//

    Controller(const sq::InputDevices& devices, const String& configPath);

    //--------------------------------------------------------//

    /// Handle a system input event.
    void handle_event(sq::Event event);

    /// Update gamepad state.
    void integrate();

    /// Refresh and access input data.
    InputFrame get_input();

    //--------------------------------------------------------//

    const sq::InputDevices& devices;

private: //===================================================//

    struct {

        int gamepad_port = -1;

        sq::Gamepad_Axis axis_move_x {-1};
        sq::Gamepad_Axis axis_move_y {-1};

        sq::Gamepad_Button button_attack {-1};
        sq::Gamepad_Button button_jump {-1};
        sq::Gamepad_Button button_shield {-1};

        sq::Keyboard_Key key_left {-1};
        sq::Keyboard_Key key_up {-1};
        sq::Keyboard_Key key_right {-1};
        sq::Keyboard_Key key_down {-1};

        sq::Keyboard_Key key_attack {-1};
        sq::Keyboard_Key key_jump {-1};
        sq::Keyboard_Key key_shield {-1};

    } config;

    //--------------------------------------------------------//

    sq::Gamepad mGamepad;

    Vec2F mPrevAxisMove;

    uint mTimeSinceZeroX = 0u;
    uint mTimeSinceZeroY = 0u;

    bool mDoneMashX = false;
    bool mDoneMashY = false;

    InputFrame mInput;

    //--------------------------------------------------------//

    /// -2 = none, -1 = record, 0+ = playing
    int mPlaybackIndex = -2;

    std::vector<InputFrame> mRecordedInput;

    //--------------------------------------------------------//

    friend DebugGui;
};

//============================================================================//

} // namespace sts
