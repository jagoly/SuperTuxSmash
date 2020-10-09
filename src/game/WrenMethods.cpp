#include "game/Action.hpp"
#include "game/Fighter.hpp"

#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/SoundEffect.hpp"

#include "main/Options.hpp"

#include <sqee/app/AudioContext.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>

using namespace sts;

//============================================================================//

template <class... Args>
inline void log_call(const Action& action, StringView str, const Args&... args)
{
    const Fighter& fighter = action.fighter;

    if (action.world.options.log_script == true)
        sq::log_debug(sq::build_string("{}/{} {:3d} | Action::", str), fighter.type, action.type, action.get_current_frame(), args...);
}

template <class... Args>
inline void log_call(const Fighter& fighter, StringView str, const Args&... args)
{
    const Action& action = *fighter.get_active_action();

    if (fighter.world.options.log_script == true)\
        sq::log_debug(sq::build_string("{}/{} {:3d} | Fighter::", str), fighter.type, action.type, action.get_current_frame(), args...);
}

//============================================================================//

void Action::wren_set_wait_until(uint frame)
{
    log_call(*this, "set_wait_until({})", frame);

    if (frame <= mCurrentFrame)
        throw wren::Exception("can't wait for {} on {}", frame, mCurrentFrame);

    mWaitingUntil = frame;
}

void Action::wren_allow_interrupt()
{
    log_call(*this, "allow_interrupt()");

    if (ActionTraits::get(type).needInterrupt == false)
        throw wren::Exception("can't interrupt this action type");

    else mStatus = ActionStatus::AllowInterrupt;
}

void Action::wren_enable_hitblobs(TinyString prefix)
{
    log_call(*this, "enable_hitblobs({})", prefix);

    bool found = false;
    for (auto& [key, blob] : mBlobs)
        if (prefix == StringView(key.c_str(), prefix.length()))
            world.enable_hitblob(&blob), found = true;

    if (found == false)
        throw wren::Exception("no hitblobs matching '{}*'", prefix);
}

void Action::wren_disable_hitblobs()
{
    log_call(*this, "disable_hitblobs()");

    world.disable_hitblobs(*this);
}

void Action::wren_emit_particles(TinyString key)
{
    log_call(*this, "emit_particles('{}')", key);

    const auto iter = mEmitters.find(key);
    if (iter == mEmitters.end())
        throw wren::Exception("invalid emitter '{}'", key);

    Emitter& emitter = iter->second;

    world.get_particle_system().generate(emitter);
}

void Action::wren_play_sound(TinyString key)
{
    log_call(*this, "play_sound('{}')", key);

    const auto iter = mSounds.find(key);
    if (iter == mSounds.end())
        throw wren::Exception("invalid sound '{}'", key);

    SoundEffect& sound = iter->second;

    if (sound.handle == nullptr)
        throw wren::Exception("could not load sound '{}'", sound.get_key());

    sound.id = world.audio.play_sound(sound.handle.get(), sq::SoundGroup::Sfx, sound.volume, false);
}

void Action::wren_cancel_sound(TinyString key)
{
    log_call(*this, "cancel_sound('{}')", key);

    const auto iter = mSounds.find(key);
    if (iter == mSounds.end())
        throw wren::Exception("invalid sound '{}'", key);

    if (iter->second.id != -1)
        world.audio.stop_sound(iter->second.id);
}

//============================================================================//

void Action::wren_set_flag_AllowNext()
{
    log_call(*this, "set_flag_AllowNext()");

    set_flag(ActionFlag::AllowNext, true);
}

void Action::wren_set_flag_AutoJab()
{
    log_call(*this, "set_flag_AutoJab()");

    set_flag(ActionFlag::AutoJab, true);
}

bool Action::wren_check_flag_AttackHeld() const
{
    log_call(*this, "check_flag_AttackHeld()");

    return check_flag(ActionFlag::AttackHeld);
}

bool Action::wren_check_flag_HitCollide() const
{
    log_call(*this, "check_flag_HitCollide()");

    return check_flag(ActionFlag::HitCollide);
}

//============================================================================//

void Fighter::wren_reset_collisions()
{
    log_call(*this, "reset_collisions()");

    world.reset_collisions(index);
}

void Fighter::wren_set_intangible(bool value)
{
    log_call(*this, "set_intangible({})", value);

    status.intangible = value;
}

void Fighter::wren_enable_hurtblob(TinyString key)
{
    log_call(*this, "enable_hurtblob({})", key);

    const auto iter = mHurtBlobs.find(key);
    if (iter == mHurtBlobs.end())
        throw wren::Exception("invalid hurt blob '{}'", key);

    world.enable_hurtblob(&iter->second);
}

void Fighter::wren_disable_hurtblob(TinyString key)
{
    log_call(*this, "enable_hurtblob({})", key);

    const auto iter = mHurtBlobs.find(key);
    if (iter == mHurtBlobs.end())
        throw wren::Exception("invalid hurt blob '{}'", key);

    world.disable_hurtblob(&iter->second);
}

void Fighter::wren_set_velocity_x(float value)
{
    log_call(*this, "set_velocity_x({})", value);

    status.velocity.x = value;
}

void Fighter::wren_set_autocancel(bool value)
{
    log_call(*this, "set_autocancel({})", value);

    status.autocancel = value;
}
