#include <sqee/debug/Logging.hpp>

#include <sqee/assert.hpp>

#include <sqee/misc/Files.hpp>

#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Event.hpp>

#include "Controller.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

template <uint N>
bool impl_inputs_enabled(std::array<int, N> inputs)
{
    for (int input : inputs) if (input < 0) return false;
    return true;
}

void Controller::load_config(const string& path)
{
    for (auto& linePair : sq::tokenise_file(path))
    {
        const auto& key = linePair.first.at(0);
        const auto& value = linePair.first.at(1);

        if (key == "joystick_id")   config.joystick_id   = stoi(value);
        if (key == "button_attack") config.button_attack = stoi(value);
        if (key == "button_jump")   config.button_jump   = stoi(value);
        if (key == "button_left")   config.button_left   = stoi(value);
        if (key == "button_right")  config.button_right  = stoi(value);
        if (key == "button_down")   config.button_down   = stoi(value);
        if (key == "button_up")     config.button_up     = stoi(value);
        if (key == "axis_move_x")   config.axis_move_x   = stoi(value);
        if (key == "axis_move_y")   config.axis_move_y   = stoi(value);
    }
}

bool Controller::handle_event(sf::Event event)
{
    if (event.type == sf::Event::JoystickButtonPressed)
    {
        if (event.joystickButton.joystickId == uint(config.joystick_id))
        {
            const int button = int(event.joystickButton.button);

            if (button == config.button_attack) mInput.press_attack = true;
            if (button == config.button_jump)   mInput.press_jump   = true;

//            sq::log_only("button: %i", button);

            //return true;
        }
    }

    if (event.type == sf::Event::JoystickMoved)
    {
        if (event.joystickButton.joystickId == uint(config.joystick_id))
        {
            const int axis = int(event.joystickMove.axis);

//            sq::log_only("axis: %i", axis);

            //return true;
        }
    }

    return false;
}

//============================================================================//


Controller::Input Controller::get_input()
{
    using JS = sf::Joystick;
    using KB = sf::Keyboard;

    //========================================================//

    if (config.joystick_id >= 0)
    {
        const uint joystick = static_cast<uint>(config.joystick_id);

        //========================================================//

        if (config.axis_move_x >= 0)
        {
            const auto axis = static_cast<JS::Axis>(config.axis_move_x);
            mInput.axis_move.x = +JS::getAxisPosition(joystick, axis) * 0.01f;
        }

        if (config.axis_move_y >= 0)
        {
            const auto axis = static_cast<JS::Axis>(config.axis_move_y);
            mInput.axis_move.y = -JS::getAxisPosition(joystick, axis) * 0.01f;
        }

        //========================================================//}

        if (config.button_attack >= 0)
            if (JS::isButtonPressed(joystick, uint(config.button_attack)))
                mInput.hold_attack = true;

        if (config.button_jump >= 0)
            if (JS::isButtonPressed(joystick, uint(config.button_jump)))
                mInput.hold_jump = true;

        //========================================================//

//        if (config.button_left >= 0)
//            if (JS::isButtonPressed(joystick, uint(config.button_left)))
//                mInput.axis_move.x -= 1.f;

//        if (config.button_right >= 0)
//            if (JS::isButtonPressed(joystick, uint(config.button_right)))
//                mInput.axis_move.x += 1.f;

//        if (config.button_down >= 0)
//            if (JS::isButtonPressed(joystick, uint(config.button_down)))
//                mInput.axis_move.y -= 1.f;

//        if (config.button_up >= 0)
//            if (JS::isButtonPressed(joystick, uint(config.button_up)))
//                mInput.axis_move.y += 1.f;

        //========================================================//

        // must normalize when using button movement
        const float axisLength = maths::length(mInput.axis_move);
        if (axisLength > 1.f) mInput.axis_move /= axisLength;

        const auto remove_deadzone = [](float value)
        {
            if (std::abs(value) < 0.1f) return 0.f * value;
            return (value - std::copysign(0.1f, value)) / 0.9f;
        };

        mInput.axis_move.x = remove_deadzone(mInput.axis_move.x);
        mInput.axis_move.y = remove_deadzone(mInput.axis_move.y);

        //========================================================//

        mInput.activate_dash |= (++mTimeSinceNotLeft < 4u && mInput.axis_move.x < -0.8f);
        mInput.activate_dash |= (++mTimeSinceNotRight < 4u && mInput.axis_move.x > +0.8f);

        if (mInput.axis_move.x > -0.2f) mTimeSinceNotLeft = 0u;
        if (mInput.axis_move.x < +0.2f) mTimeSinceNotRight = 0u;
    }

    //========================================================//

    auto result = mInput;
    mInput = Input();

    return result;
}
