#pragma once

#include "setup.hpp"

#include <sqee/app/Event.hpp>
#include <sqee/app/Gamepad.hpp>
#include <sqee/app/InputDevices.hpp>

namespace sts {

//============================================================================//

/// One frame of input from a controller.
struct InputFrame final
{
    bool pressAttack = false;
    bool pressSpecial = false;
    bool pressJump = false;
    bool pressShield = false;

    bool holdAttack = false;
    bool holdSpecial = false;
    bool holdJump = false;
    bool holdShield = false;

    /// -4, -3, -2, -1, 0, +1, +2, +3, +4
    int8_t intX = 0, intY = 0;

    /// set for one frame if you quickly push all the way
    int8_t mashX = 0, mashY = 0;

    /// used for starting smashes and dodges
    int8_t modX = 0, modY = 0;

    /// relative to fighter facing
    int8_t relIntX = 0, relMashX = 0, relModX = 0;

    /// -1.0, -0.75, -0.5, -0.25, 0.0, +0.25, +0.5, +0.75, +1.0
    float floatX = 0.f, floatY = 0.f;

    /// called by fighter on tick, and when facing changes
    void set_relative_x(int8_t facing)
    {
        relIntX = intX * facing;
        relMashX = mashX * facing;
        relModX = modX * facing;
    }
};

//============================================================================//

/// Buffered input from the last few frames.
struct InputHistory final
{
    // frames are stored from most to least recent
    // entry zero is always valid and contains the most recent state
    sq::StackVector<InputFrame, CMD_BUFFER_SIZE> frames;

    // if true, then make iterate return null and erase the vector next tick
    // defer actually clearing since we need the previous frame to build a new one
    bool cleared = false;

    std::optional<uint8_t> wren_iterate(std::optional<uint8_t> index)
    {
        // no checks required as long as we don't call .iterate(_) manually
        if (index == std::nullopt) return uint8_t(0u);
        if (cleared || ++(*index) == frames.size()) return std::nullopt;
        return *index;
    }

    InputFrame* wren_iterator_value(uint8_t index)
    {
        // no checks required as long as we don't call .iteratorValue(_) manually
        return &frames[index];
    }
};

//============================================================================//

class Controller final : sq::NonCopyable
{
public: //====================================================//

    /// List of axis, button, and key mappings.
    struct Config
    {
        int gamepad_port = -1;

        sq::Gamepad_Axis axis_move_x {-1};
        sq::Gamepad_Axis axis_move_y {-1};

        sq::Gamepad_Button button_attack {-1};
        sq::Gamepad_Button button_special {-1};
        sq::Gamepad_Button button_jump {-1};
        sq::Gamepad_Button button_shield {-1};

        sq::Keyboard_Key key_left {-1};
        sq::Keyboard_Key key_up {-1};
        sq::Keyboard_Key key_right {-1};
        sq::Keyboard_Key key_down {-1};

        sq::Keyboard_Key key_attack {-1};
        sq::Keyboard_Key key_special {-1};
        sq::Keyboard_Key key_jump {-1};
        sq::Keyboard_Key key_shield {-1};
    };

    /// Clone of sq::Gamepad, but for the keyboard.
    struct Keyboard
    {
        std::array<bool, 4> buttons {};
        std::array<bool, 4> pressed {};
        std::array<bool, 4> released {};
        std::array<float, 2> axes {};
    };

    //--------------------------------------------------------//

    Controller(const sq::InputDevices& devices, const String& configPath);

    const sq::InputDevices& devices;

    Config config;

    InputHistory history;

    //--------------------------------------------------------//

    /// Poll gamepad and/or keyboard state.
    void refresh();

    /// Build a new frame of input.
    void tick();

    //-- wren methods ----------------------------------------//

    InputFrame* wren_get_input() { return &history.frames.front(); }

    void wren_clear_history() { history.cleared = true; }

private: //===================================================//

    sq::Gamepad mGamepad;
    Keyboard mKeyboard;

    //--------------------------------------------------------//

    uint8_t mTimeSinceZeroX = 0u;
    uint8_t mTimeSinceZeroY = 0u;

    bool mDoneMashX = false;
    bool mDoneMashY = false;

    //--------------------------------------------------------//

    std::vector<InputFrame> mRecordedInput;

    /// -2 = none, -1 = record, 0+ = playing
    int mPlaybackIndex = -2;

    bool mGamepadEnabled = false;
    bool mKeyboardEnabled = false;
    bool mKeyboardMode = false;

    bool mPolledSinceLastTick = false;

    //--------------------------------------------------------//

    friend DebugGui;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::InputFrame)
WRENPLUS_TRAITS_HEADER(sts::InputHistory)
WRENPLUS_TRAITS_HEADER(sts::Controller)
