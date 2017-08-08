#include <sqee/misc/Json.hpp>

#include "Controller.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Controller::Controller(uint8_t index, const sq::InputDevices& devices, string configPath)
    : index(index), mDevices(devices)
{
    const auto root = sq::parse_json(configPath);

    config.gamepad_port  = static_cast<int>                (int(root.at("gamepad_port")));
    config.stick_move    = static_cast<sq::Gamepad_Stick>  (int(root.at("stick_move")));
    config.button_attack = static_cast<sq::Gamepad_Button> (int(root.at("button_attack")));
    config.button_jump   = static_cast<sq::Gamepad_Button> (int(root.at("button_jump")));
    config.key_left      = static_cast<sq::Keyboard_Key>   (int(root.at("key_left")));
    config.key_up        = static_cast<sq::Keyboard_Key>   (int(root.at("key_up")));
    config.key_right     = static_cast<sq::Keyboard_Key>   (int(root.at("key_right")));
    config.key_down      = static_cast<sq::Keyboard_Key>   (int(root.at("key_down")));
    config.key_attack    = static_cast<sq::Keyboard_Key>   (int(root.at("key_attack")));
    config.key_jump      = static_cast<sq::Keyboard_Key>   (int(root.at("key_jump")));
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
        }
    }

    if (event.type == sq::Event::Type::Keyboard_Press)
    {
        const auto key = event.data.keyboard.key;

        if (key == config.key_attack) mInput.press_attack = true;
        if (key == config.key_jump)   mInput.press_jump   = true;
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

    //--------------------------------------------------------//

    // must normalize when using key or button input
    const float axisLength = maths::length(mInput.axis_move);
    if (axisLength > 1.f) mInput.axis_move /= axisLength;

    const auto remove_deadzone = [](float value)
    {
        if (std::abs(value) < 0.1f) return 0.f * value;
        return (value - std::copysign(0.1f, value)) / 0.9f;
    };

    mInput.axis_move.x = remove_deadzone(mInput.axis_move.x);
    mInput.axis_move.y = remove_deadzone(mInput.axis_move.y);

    //--------------------------------------------------------//

    mInput.activate_dash |= (++mTimeSinceNotLeft < 4u && mInput.axis_move.x < -0.8f);
    mInput.activate_dash |= (++mTimeSinceNotRight < 4u && mInput.axis_move.x > +0.8f);

    if (mInput.axis_move.x > -0.2f) mTimeSinceNotLeft = 0u;
    if (mInput.axis_move.x < +0.2f) mTimeSinceNotRight = 0u;

    //--------------------------------------------------------//

    Input result = mInput;
    mInput = Input();

    return result;
}
