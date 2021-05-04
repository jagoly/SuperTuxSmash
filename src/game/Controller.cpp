#include "game/Controller.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

Controller::Controller(const sq::InputDevices& devices, const String& configPath)
    : devices(devices)
{
    const JsonValue root = sq::parse_json_from_file(configPath);

    config.gamepad_port  = static_cast<int>                (int(root.at("gamepad_port")));
    config.axis_move_x   = static_cast<sq::Gamepad_Axis>   (int(root.at("axis_move_x")));
    config.axis_move_y   = static_cast<sq::Gamepad_Axis>   (int(root.at("axis_move_y")));
    config.button_attack = static_cast<sq::Gamepad_Button> (int(root.at("button_attack")));
    config.button_jump   = static_cast<sq::Gamepad_Button> (int(root.at("button_jump")));
    config.button_shield = static_cast<sq::Gamepad_Button> (int(root.at("button_shield")));
    config.key_left      = static_cast<sq::Keyboard_Key>   (int(root.at("key_left")));
    config.key_up        = static_cast<sq::Keyboard_Key>   (int(root.at("key_up")));
    config.key_right     = static_cast<sq::Keyboard_Key>   (int(root.at("key_right")));
    config.key_down      = static_cast<sq::Keyboard_Key>   (int(root.at("key_down")));
    config.key_attack    = static_cast<sq::Keyboard_Key>   (int(root.at("key_attack")));
    config.key_jump      = static_cast<sq::Keyboard_Key>   (int(root.at("key_jump")));
    config.key_shield    = static_cast<sq::Keyboard_Key>   (int(root.at("key_shield")));
}

//============================================================================//

void Controller::handle_event(sq::Event event)
{
    // don't handle events while playing recording
    if (mPlaybackIndex >= 0) return;

    // todo: fix keyboard input to handle press/release the same way as gamepad
    // don't really care for now since it only makes a difference in slo-mo
    if (event.type == sq::Event::Type::Keyboard_Press && event.data.keyboard.key != sq::Keyboard_Key::Unknown)
    {
        const auto key = event.data.keyboard.key;

        if (key == config.key_attack) mInput.press_attack = mInput.hold_attack = true;
        if (key == config.key_jump)   mInput.press_jump   = mInput.hold_jump   = true;
        if (key == config.key_shield) mInput.press_shield = mInput.hold_shield = true;
    }
}

//============================================================================//

void Controller::integrate()
{
    // don't handle input while playing recording
    if (mPlaybackIndex >= 0) return;

    // ask operating system for updated gamepad state
    if (config.gamepad_port >= 0)
        mGamepad.integrate(devices.poll_gamepad_state(config.gamepad_port));
}

//============================================================================//

InputFrame Controller::get_input()
{
    if (mPlaybackIndex >= 0)
    {
        if (mPlaybackIndex < int(mRecordedInput.size()))
            return mRecordedInput[size_t(mPlaybackIndex++)];

        mPlaybackIndex = -2;
    }

    //--------------------------------------------------------//

    // Note that if a button was pressed since last tick, hold will be set to true this tick.
    // This applies even if the button was no longer held on the most recent poll.
    // Likewise, if a button was released since last tick, then hold will be set to false.
    // As a result, press can never be true for multiple ticks in a row.

    if (config.gamepad_port >= 0)
    {
        if (config.button_attack != sq::Gamepad_Button::Unknown)
        {
            if (mInput.hold_attack == true)
                mInput.hold_attack = mGamepad.buttons[int8_t(config.button_attack)] && !mGamepad.released[int8_t(config.button_attack)];
            else
                mInput.press_attack = mInput.hold_attack = mGamepad.pressed[int8_t(config.button_attack)];
        }

        if (config.button_jump != sq::Gamepad_Button::Unknown)
        {
            if (mInput.hold_jump == true)
                mInput.hold_jump = mGamepad.buttons[int8_t(config.button_jump)] && !mGamepad.released[int8_t(config.button_jump)];
            else
                mInput.press_jump = mInput.hold_jump = mGamepad.pressed[int8_t(config.button_jump)];
        }

        if (config.button_shield != sq::Gamepad_Button::Unknown)
        {
            if (mInput.hold_shield == true)
                mInput.hold_shield = mGamepad.buttons[int8_t(config.button_shield)] && !mGamepad.released[int8_t(config.button_shield)];
            else
                mInput.press_shield = mInput.hold_shield = mGamepad.pressed[int8_t(config.button_shield)];
        }

        if (config.axis_move_x != sq::Gamepad_Axis::Unknown)
            mInput.float_axis.x = mGamepad.axes[int8_t(config.axis_move_x)];

        if (config.axis_move_y != sq::Gamepad_Axis::Unknown)
            mInput.float_axis.y = mGamepad.axes[int8_t(config.axis_move_y)];
    }

    //--------------------------------------------------------//

    if (config.key_left != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_left))
            mInput.float_axis.x -= 1.f;

    if (config.key_up != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_up))
            mInput.float_axis.y += 1.f;

    if (config.key_right != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_right))
            mInput.float_axis.x += 1.f;

    if (config.key_down != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_down))
            mInput.float_axis.y -= 1.f;

    if (config.key_attack != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_attack))
            mInput.hold_attack = true;

    if (config.key_jump != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_jump))
            mInput.hold_jump = true;

    if (config.key_shield != sq::Keyboard_Key::Unknown)
        if (devices.is_pressed(config.key_shield))
            mInput.hold_shield = true;

    //--------------------------------------------------------//

    DISABLE_WARNING_FLOAT_EQUALITY();

    //--------------------------------------------------------//

    const auto remove_deadzone_and_discretise = [](float value) -> float
    {
        const float absValue = std::abs(value);

        if (absValue < 0.2f) return std::copysign(0.f, value);
        if (absValue > 0.7f) return std::copysign(1.f, value);

        return std::copysign(0.5f, value);
    };

    const auto clamp_difference = [](float previous, float target) -> float
    {
        if (previous == -0.5f) return maths::clamp(target, -1.0f, -0.0f);
        if (previous == +0.5f) return maths::clamp(target, +0.0f, +1.0f);

        if (previous == -1.0f) return maths::min(target, -0.5f);
        if (previous == +1.0f) return maths::max(target, +0.5f);

        return maths::clamp(target, -0.5f, +0.5f);
    };

    //--------------------------------------------------------//

    mInput.float_axis.x = remove_deadzone_and_discretise(mInput.float_axis.x);
    mInput.float_axis.y = remove_deadzone_and_discretise(mInput.float_axis.y);

    mInput.float_axis.x = clamp_difference(mPrevAxisMove.x, mInput.float_axis.x);
    mInput.float_axis.y = clamp_difference(mPrevAxisMove.y, mInput.float_axis.y);

    //--------------------------------------------------------//

    // todo: should really go from int to float

    mInput.int_axis.x = int8_t(mInput.float_axis.x * 2.f);
    mInput.int_axis.y = int8_t(mInput.float_axis.y * 2.f);

    //--------------------------------------------------------//

    if (mInput.float_axis.x != 0.f && ++mTimeSinceZeroX <= 4u)
    {
        if (!mDoneMashX && mInput.float_axis.x == -1.f) mDoneMashX = (mInput.mash_axis.x = -1);
        if (!mDoneMashX && mInput.float_axis.x == +1.f) mDoneMashX = (mInput.mash_axis.x = +1);
        mInput.mod_axis.x = std::signbit(mInput.float_axis.x) ? -1 : +1;
    }

    if (mInput.float_axis.y != 0.f && ++mTimeSinceZeroY <= 4u)
    {
        if (!mDoneMashY && mInput.float_axis.y == -1.f) mDoneMashY = (mInput.mash_axis.y = -1);
        if (!mDoneMashY && mInput.float_axis.y == +1.f) mDoneMashY = (mInput.mash_axis.y = +1);
        mInput.mod_axis.y = std::signbit(mInput.float_axis.y) ? -1 : +1;
    }

    if (mInput.float_axis.x == 0.f) mDoneMashX = (mTimeSinceZeroX = 0u);
    if (mInput.float_axis.y == 0.f) mDoneMashY = (mTimeSinceZeroY = 0u);

    //--------------------------------------------------------//

    ENABLE_WARNING_FLOAT_EQUALITY();

    //--------------------------------------------------------//

    mInput.norm_axis = { mInput.int_axis.x, mInput.int_axis.y };

    if (mInput.norm_axis.x == -2) mInput.norm_axis.x = -1;
    else if (mInput.norm_axis.x == +2) mInput.norm_axis.x = +1;

    if (mInput.norm_axis.y == -2) mInput.norm_axis.y = -1;
    else if (mInput.norm_axis.y == +2) mInput.norm_axis.y = +1;

    //--------------------------------------------------------//

    mPrevAxisMove = mInput.float_axis;

    InputFrame result = mInput;

    // need to keep button hold values for next tick
    mInput.press_attack = mInput.press_jump = mInput.press_shield = false;
    mInput.mash_axis = mInput.mod_axis = {};

    mGamepad.finish_tick();

    if (mPlaybackIndex == -1)
        mRecordedInput.push_back(result);

    return result;
}
