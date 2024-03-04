#include "game/Controller.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

Controller::Controller(const sq::InputDevices& devices, const String& configPath)
    : devices(devices)
{
    const auto document = JsonDocument::parse_file(configPath);
    const auto json = document.root().as<JsonObject>();

    const auto get_config_value = [&](StringView key, auto& ref) -> bool
    {
        if (const auto j = json[key]; !j.is_null()) ref = j.as_auto();
        return int8_t(ref) != -1;
    };

    mGamepadEnabled = get_config_value("gamepad_port", config.gamepad_port);
    mGamepadEnabled &= get_config_value("axis_move_x", config.axis_move_x);
    mGamepadEnabled &= get_config_value("axis_move_y", config.axis_move_y);
    mGamepadEnabled &= get_config_value("button_attack", config.button_attack);
    mGamepadEnabled &= get_config_value("button_special", config.button_special);
    mGamepadEnabled &= get_config_value("button_jump", config.button_jump);
    mGamepadEnabled &= get_config_value("button_shield", config.button_shield);
    mGamepadEnabled &= get_config_value("button_grab", config.button_grab);

    if (config.gamepad_port >= 0 && mGamepadEnabled == false)
        sq::log_warning("invalid Gamepad configuration in '{}'", configPath);

    mKeyboardEnabled = true;
    mKeyboardEnabled &= get_config_value("key_left", config.key_left);
    mKeyboardEnabled &= get_config_value("key_up", config.key_up);
    mKeyboardEnabled &= get_config_value("key_right", config.key_right);
    mKeyboardEnabled &= get_config_value("key_down", config.key_down);
    mKeyboardEnabled &= get_config_value("key_attack", config.key_attack);
    mKeyboardEnabled &= get_config_value("key_special", config.key_special);
    mKeyboardEnabled &= get_config_value("key_jump", config.key_jump);
    mKeyboardEnabled &= get_config_value("key_shield", config.key_shield);
    mKeyboardEnabled &= get_config_value("key_grab", config.key_grab);

    mKeyboardMode = mKeyboardEnabled;

    // make sure we have a "previous" value
    history.frames.emplace_back();
}

//============================================================================//

void Controller::refresh()
{
    // don't handle input while playing back a recording
    if (mPlaybackIndex >= 0)
        return;

    mPolledSinceLastTick = true;

    //--------------------------------------------------------//

    // ask the operating system for updated gamepad state
    if (mGamepadEnabled == true)
    {
        if (devices.check_gamepad_connected(config.gamepad_port))
        {
            mGamepad.integrate(devices.poll_gamepad_state(config.gamepad_port));
            mKeyboardMode = false;
        }
        else // gamepad is configured but not connected
        {
            mGamepad = {};
            mKeyboardMode = mKeyboardEnabled;
        }
    }

    //--------------------------------------------------------//

    // do the same thing but for keyboard input
    if (mKeyboardMode == true)
    {
        const auto update_button = [this](sq::Keyboard_Key key, uint8_t index)
        {
            const bool latest = devices.is_pressed(key);

            mKeyboard.pressed[index] |= !mKeyboard.buttons[index] && latest;
            mKeyboard.released[index] |= mKeyboard.buttons[index] && !latest;
            mKeyboard.buttons[index] = latest;
        };

        const auto update_axis = [this](sq::Keyboard_Key negKey, sq::Keyboard_Key posKey, uint8_t index)
        {
            float latest = 0.f;

            if (devices.is_pressed(negKey)) latest -= 1.f;
            if (devices.is_pressed(posKey)) latest += 1.f;

            if (std::abs(latest) >= std::abs(mKeyboard.axes[index]))
                mKeyboard.axes[index] = latest;
        };

        update_button(config.key_attack, 0u);
        update_button(config.key_special, 1u);
        update_button(config.key_jump, 2u);
        update_button(config.key_shield, 3u);
        update_button(config.key_grab, 4u);
        update_axis(config.key_left, config.key_right, 0u);
        update_axis(config.key_down, config.key_up, 1u);
    }
}

//============================================================================//

void Controller::tick()
{
    // make sure we have polled at least once
    if (mPolledSinceLastTick == false)
        refresh();

    mPolledSinceLastTick = false;

    if (history.frames.full() == true)
        history.frames.pop_back();

    //--------------------------------------------------------//

    // playing back some recorded input
    if (mPlaybackIndex >= 0)
    {
        if (mPlaybackIndex < int(mRecordedInput.size()))
        {
            const InputFrame& record = mRecordedInput[mPlaybackIndex++];

            if (history.cleared == true)
            {
                history.frames = { record };
                history.cleared = false;
            }
            else history.frames.insert(history.frames.begin(), record);

            return; // don't bother getting a new frame
        }

        mPlaybackIndex = -2;
    }

    //--------------------------------------------------------//

    // default construct a new frame at the beginning of the vector
    InputFrame& current = *history.frames.emplace(history.frames.begin());

    // we use the previous frame to sanitise button press and hold values
    const InputFrame& previous = history.frames[1];

    // input axis without any discretisation
    Vec2F rawAxis = { 0.f, 0.f };

    //--------------------------------------------------------//

    const auto update_button = [](const auto& state, auto index, bool previousHold, bool& press, bool& hold)
    {
        // state.pressed always results in exactly one frame of press = true
        // state.released always results in at least one frame of hold = false
        if (previousHold == true)
            hold = state.buttons[int8_t(index)] && !state.released[int8_t(index)];
        else
            press = hold = state.pressed[int8_t(index)];
    };

    const auto update_raw_axis = [](const auto& state, auto index, float& axis)
    {
        // the maximum absolute value from all polls since last tick
        axis = state.axes[int8_t(index)];
    };

    if (mKeyboardMode == false && mGamepadEnabled == true)
    {
        update_button(mGamepad, config.button_attack, previous.holdAttack, current.pressAttack, current.holdAttack);
        update_button(mGamepad, config.button_special, previous.holdSpecial, current.pressSpecial, current.holdSpecial);
        update_button(mGamepad, config.button_jump, previous.holdJump, current.pressJump, current.holdJump);
        update_button(mGamepad, config.button_shield, previous.holdShield, current.pressShield, current.holdShield);
        update_button(mGamepad, config.button_grab, previous.holdGrab, current.pressGrab, current.holdGrab);

        update_raw_axis(mGamepad, config.axis_move_x, rawAxis.x);
        update_raw_axis(mGamepad, config.axis_move_y, rawAxis.y);
    }

    if (mKeyboardMode == true)
    {
        update_button(mKeyboard, 0u, previous.holdAttack, current.pressAttack, current.holdAttack);
        update_button(mKeyboard, 1u, previous.holdSpecial, current.pressSpecial, current.holdSpecial);
        update_button(mKeyboard, 2u, previous.holdJump, current.pressJump, current.holdJump);
        update_button(mKeyboard, 3u, previous.holdShield, current.pressShield, current.holdShield);
        update_button(mKeyboard, 4u, previous.holdGrab, current.pressGrab, current.holdGrab);

        update_raw_axis(mKeyboard, 0u, rawAxis.x);
        update_raw_axis(mKeyboard, 1u, rawAxis.y);
    }

    //--------------------------------------------------------//

    // todo: add values to config file (calibration)
    const auto compute_int_axis = [](float raw) -> int8_t
    {
        if (raw < -0.8f) return -4;
        if (raw < -0.6f) return -3;
        if (raw < -0.4f) return -2;
        if (raw < -0.2f) return -1;
        if (raw > +0.8f) return +4;
        if (raw > +0.6f) return +3;
        if (raw > +0.4f) return +2;
        if (raw > +0.2f) return +1;
        return 0;
    };

    const auto update_mash_mod = [](int8_t prevAxis, int8_t axis, uint8_t& timeSinceZero, bool& doneMash, int8_t& mash, int8_t& mod)
    {
        if (prevAxis * axis <= 0) timeSinceZero = 0u, doneMash = false;

        if (timeSinceZero < 6)
        {
            if (doneMash == false)
            {
                if (axis == -4) mash = -1, doneMash = true;
                if (axis == +4) mash = +1, doneMash = true;
            }
            if (axis <= -3) mod = -1;
            if (axis >= +3) mod = +1;
        }

        if (timeSinceZero < 255u) ++timeSinceZero;
    };

    current.intX = compute_int_axis(rawAxis.x);
    current.intY = compute_int_axis(rawAxis.y);

    update_mash_mod(previous.intX, current.intX, mTimeSinceZeroX, mDoneMashX, current.mashX, current.modX);
    update_mash_mod(previous.intY, current.intY, mTimeSinceZeroY, mDoneMashY, current.mashY, current.modY);

    current.floatX = float(current.intX) * 0.25f;
    current.floatY = float(current.intY) * 0.25f;

    //--------------------------------------------------------//

    mGamepad.finish_tick();
    mKeyboard.pressed.fill(false);
    mKeyboard.released.fill(false);
    mKeyboard.axes.fill(0.f);

    if (history.cleared == true)
    {
        // keep the new frame that we just created
        history.frames.erase(history.frames.begin()+1, history.frames.end());
        history.cleared = false;
    }

    // append the new frame to the recording
    if (mPlaybackIndex == -1)
        mRecordedInput.push_back(current);
}
