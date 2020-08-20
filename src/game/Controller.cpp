#include "game/Controller.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

Controller::Controller(const sq::InputDevices& devices, const String& configPath)
    : devices(devices)
{
    const JsonValue root = sq::parse_json_from_file(configPath);

    config.gamepad_port  = static_cast<int>                (int(root.at("gamepad_port")));
    config.stick_move    = static_cast<sq::Gamepad_Stick>  (int(root.at("stick_move")));
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

    if (event.type == sq::Event::Type::Gamepad_Press)
    {
        if (int(event.data.gamepad.port) == config.gamepad_port)
        {
            const auto button = event.data.gamepad.button;

            if (button == config.button_attack) mInput.press_attack = true;
            if (button == config.button_jump)   mInput.press_jump   = true;
            if (button == config.button_shield) mInput.press_shield = true;
        }
    }

    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        const auto key = event.data.keyboard.key;

        if (key == config.key_attack) mInput.press_attack = true;
        if (key == config.key_jump)   mInput.press_jump   = true;
        if (key == config.key_shield) mInput.press_shield = true;
    }
}

//============================================================================//

InputFrame Controller::get_input()
{
    using Stick = sq::Gamepad_Stick;
    using Button = sq::Gamepad_Button;
    using Key = sq::Keyboard_Key;

    //--------------------------------------------------------//

    if (mPlaybackIndex >= 0)
    {
        if (mPlaybackIndex < int(mRecordedInput.size()))
            return mRecordedInput[size_t(mPlaybackIndex++)];

        mPlaybackIndex = -2;
    }

    //--------------------------------------------------------//

    if (config.gamepad_port >= 0)
    {
        const int port = config.gamepad_port;

        if (config.stick_move != Stick::Unknown)
            mInput.float_axis = devices.get_stick_pos(port, config.stick_move);

        if (config.button_attack != Button::Unknown)
            if (devices.is_pressed(port, config.button_attack))
                mInput.hold_attack = true;

        if (config.button_jump != Button::Unknown)
            if (devices.is_pressed(port, config.button_jump))
                mInput.hold_jump = true;

        if (config.button_shield != Button::Unknown)
            if (devices.is_pressed(port, config.button_shield))
                mInput.hold_shield = true;
    }

    //--------------------------------------------------------//

    if (config.key_left != Key::Unknown)
        if (devices.is_pressed(config.key_left))
            mInput.float_axis.x -= 1.f;

    if (config.key_up != Key::Unknown)
        if (devices.is_pressed(config.key_up))
            mInput.float_axis.y += 1.f;

    if (config.key_right != Key::Unknown)
        if (devices.is_pressed(config.key_right))
            mInput.float_axis.x += 1.f;

    if (config.key_down != Key::Unknown)
        if (devices.is_pressed(config.key_down))
            mInput.float_axis.y -= 1.f;

    if (config.key_attack != Key::Unknown)
        if (devices.is_pressed(config.key_attack))
            mInput.hold_attack = true;

    if (config.key_jump != Key::Unknown)
        if (devices.is_pressed(config.key_jump))
            mInput.hold_jump = true;

    if (config.key_shield != Key::Unknown)
        if (devices.is_pressed(config.key_shield))
            mInput.hold_shield = true;

    //--------------------------------------------------------//

    DISABLE_WARNING_FLOAT_EQUALITY;

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

    ENABLE_WARNING_FLOAT_EQUALITY;

    //--------------------------------------------------------//

    mInput.norm_axis = { mInput.int_axis.x, mInput.int_axis.y };

    if (mInput.norm_axis.x == -2) mInput.norm_axis.x = -1;
    else if (mInput.norm_axis.x == +2) mInput.norm_axis.x = +1;

    if (mInput.norm_axis.y == -2) mInput.norm_axis.y = -1;
    else if (mInput.norm_axis.y == +2) mInput.norm_axis.y = +1;

    //--------------------------------------------------------//

    mPrevAxisMove = mInput.float_axis;

    InputFrame result = mInput;
    mInput = InputFrame();

    if (mPlaybackIndex == -1)
        mRecordedInput.push_back(result);

    return result;
}
