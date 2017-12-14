#include <sqee/debug/Logging.hpp>

#include <sqee/misc/Json.hpp>

#include "DebugGlobals.hpp"

#include "Controller.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Controller::Controller(const sq::InputDevices& devices, string configPath)
    : mDevices(devices)
{
    const auto root = sq::parse_json_from_file(configPath);

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
    if (event.type == sq::Event::Type::Gamepad_Press)
    {
        if (int(event.data.gamepad.port) == config.gamepad_port)
        {
            const auto button = event.data.gamepad.button;

            if (button == config.button_attack) mInput.press_attack = true;
            if (button == config.button_jump)   mInput.press_jump   = true;
            if (button == config.button_shield) mInput.press_shield   = true;
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

Controller::Input Controller::get_input()
{
    using Stick = sq::Gamepad_Stick;
    using Button = sq::Gamepad_Button;
    using Key = sq::Keyboard_Key;

    //--------------------------------------------------------//

    if (config.gamepad_port >= 0)
    {
        const int port = config.gamepad_port;

        if (config.stick_move != Stick::Unknown)
            mInput.axis_move = mDevices.get_stick_pos(port, config.stick_move);

        if (config.button_attack != Button::Unknown)
            if (mDevices.is_pressed(port, config.button_attack))
                mInput.hold_attack = true;

        if (config.button_jump != Button::Unknown)
            if (mDevices.is_pressed(port, config.button_jump))
                mInput.hold_jump = true;

        if (config.button_shield != Button::Unknown)
            if (mDevices.is_pressed(port, config.button_shield))
                mInput.hold_shield = true;
    }

    //--------------------------------------------------------//

    if (config.key_left != Key::Unknown)
        if (mDevices.is_pressed(config.key_left))
            mInput.axis_move.x -= 1.f;

    if (config.key_up != Key::Unknown)
        if (mDevices.is_pressed(config.key_up))
            mInput.axis_move.y += 1.f;

    if (config.key_right != Key::Unknown)
        if (mDevices.is_pressed(config.key_right))
            mInput.axis_move.x += 1.f;

    if (config.key_down != Key::Unknown)
        if (mDevices.is_pressed(config.key_down))
            mInput.axis_move.y -= 1.f;

    if (config.key_attack != Key::Unknown)
        if (mDevices.is_pressed(config.key_attack))
            mInput.hold_attack = true;

    if (config.key_jump != Key::Unknown)
        if (mDevices.is_pressed(config.key_jump))
            mInput.hold_jump = true;

    if (config.key_shield != Key::Unknown)
        if (mDevices.is_pressed(config.key_shield))
            mInput.hold_shield = true;

    //--------------------------------------------------------//

    DISABLE_FLOAT_EQUALITY_WARNING;

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

    mInput.axis_move.x = remove_deadzone_and_discretise(mInput.axis_move.x);
    mInput.axis_move.y = remove_deadzone_and_discretise(mInput.axis_move.y);

    mInput.axis_move.x = clamp_difference(mPrevAxisMove.x, mInput.axis_move.x);
    mInput.axis_move.y = clamp_difference(mPrevAxisMove.y, mInput.axis_move.y);

    //--------------------------------------------------------//

    if (mInput.axis_move.x != 0.f && ++mTimeSinceZeroX <= 4u)
    {
        if (!mDoneMashX && mInput.axis_move.x == -1.f) mDoneMashX = (mInput.mash_axis_x = -1);
        if (!mDoneMashX && mInput.axis_move.x == +1.f) mDoneMashX = (mInput.mash_axis_x = +1);
        mInput.mod_axis_x = std::signbit(mInput.axis_move.x) ? -1 : +1;
    }

    if (mInput.axis_move.y != 0.f && ++mTimeSinceZeroY <= 4u)
    {
        if (!mDoneMashY && mInput.axis_move.y == -1.f) mDoneMashY = (mInput.mash_axis_y = -1);
        if (!mDoneMashY && mInput.axis_move.y == +1.f) mDoneMashY = (mInput.mash_axis_y = +1);
        mInput.mod_axis_y = std::signbit(mInput.axis_move.y) ? -1 : +1;
    }

    if (mInput.axis_move.x == 0.f) mDoneMashX = (mTimeSinceZeroX = 0u);
    if (mInput.axis_move.y == 0.f) mDoneMashY = (mTimeSinceZeroY = 0u);

    //--------------------------------------------------------//

    ENABLE_FLOAT_EQUALITY_WARNING;

    //--------------------------------------------------------//

    mPrevAxisMove = mInput.axis_move;

    Input result = mInput;
    mInput = Input();

    #ifdef SQEE_DEBUG
    if (dbg.disableInput) result = Input();
    #endif

    return result;
}
