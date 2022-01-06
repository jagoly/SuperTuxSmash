#include "game/Fighter.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/Stage.hpp"
#include "game/World.hpp"

#include "main/Options.hpp"

#include "game/Controller.hpp"
#include "game/EffectSystem.hpp"
#include "game/Emitter.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/ParticleSystem.hpp"
#include "game/SoundEffect.hpp"
#include "game/VisualEffect.hpp"

#include <sqee/app/AudioContext.hpp>
#include <sqee/debug/Logging.hpp>

using namespace sts;

//============================================================================//

void Fighter::wren_log(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {}: {}", index, message);
}

//----------------------------------------------------------------------------//

void Fighter::wren_cxx_assign_action(SmallString key)
{
    const auto iter = mActions.find(key);
    if (iter == mActions.end())
        throw wren::Exception("invalid action '{}'", key);

    activeAction = &iter->second;
}

void Fighter::wren_cxx_assign_state(TinyString key)
{
    const auto iter = mStates.find(key);
    if (iter == mStates.end())
        throw wren::Exception("invalid state '{}'", key);

    activeState = &iter->second;
}

//----------------------------------------------------------------------------//

void Fighter::wren_reverse_facing_auto()
{
    variables.facing = -variables.facing;

    for (InputFrame& frame : controller->history.frames)
        frame.set_relative_x(variables.facing);

    mRotateMode = RotateMode::Auto;
}

void Fighter::wren_reverse_facing_instant()
{
    wren_reverse_facing_auto();

    previous.rotation = QuatF(0.f, 0.25f * float(variables.facing), 0.f);
    current.rotation = previous.rotation;
}

void Fighter::wren_reverse_facing_slow(bool clockwise, uint8_t time)
{
    wren_reverse_facing_auto();

    if (clockwise == false) mRotateMode = RotateMode::Slow;
    else mRotateMode = RotateMode::Slow | RotateMode::Clockwise;

    mRotateSlowTime = time;
    mRotateSlowProgress = 0u;
}

void Fighter::wren_reverse_facing_animated(bool clockwise)
{
    wren_reverse_facing_auto();

    if (clockwise == false) mRotateMode = RotateMode::Animation;
    else mRotateMode = RotateMode::Animation | RotateMode::Clockwise;

    mFadeStartRotation = QuatF(0.f, -0.25f * float(variables.facing), 0.f);
}

//----------------------------------------------------------------------------//

bool Fighter::wren_attempt_ledge_catch()
{
    if (variables.ledge != nullptr)
        throw wren::Exception("already have a ledge");

    if (variables.noCatchTime != 0u || variables.velocity.y > 0.f)
        return false;

    variables.ledge = world.get_stage().find_ledge (
        localDiamond, variables.position, variables.facing, controller->history.frames.front().intX
    );

    return variables.ledge != nullptr;
}

//----------------------------------------------------------------------------//

void Fighter::wren_play_animation(SmallString key, uint fade, bool fromStart)
{
    const auto iter = mAnimations.find(key);
    if (iter == mAnimations.end())
        throw wren::Exception("invalid animation '{}'", key);

    if (fromStart == false && iter->second.anim.frameCount <= mAnimTimeDiscrete)
        throw wren::Exception("current frame is {}, but animation only has {}", mAnimTimeDiscrete, iter->second.anim.frameCount);

    play_animation(iter->second, fade, fromStart);
}

void Fighter::wren_set_next_animation(SmallString key, uint fade)
{
    const auto iter = mAnimations.find(key);
    if (iter == mAnimations.end())
        throw wren::Exception("invalid animation '{}'", key);

    if (mAnimation != nullptr && mAnimation->loop == true)
        throw wren::Exception("already playing a loop animation, '{}'", mAnimation->get_key());

    if (mAnimation == &iter->second)
        throw wren::Exception("specified animation already playing");

    set_next_animation(iter->second, fade);
}

//----------------------------------------------------------------------------//

void Fighter::wren_reset_collisions()
{
    world.reset_collisions(index);
}

void Fighter::wren_enable_hurtblob(TinyString key)
{
    const auto iter = mHurtBlobs.find(key);
    if (iter == mHurtBlobs.end())
        throw wren::Exception("invalid hurt blob '{}'", key);

    world.enable_hurtblob(&iter->second);
}

void Fighter::wren_disable_hurtblob(TinyString key)
{
    const auto iter = mHurtBlobs.find(key);
    if (iter == mHurtBlobs.end())
        throw wren::Exception("invalid hurt blob '{}'", key);

    world.disable_hurtblob(&iter->second);
}

//----------------------------------------------------------------------------//

void Fighter::wren_play_sound(SmallString key)
{
    const auto iter = mSounds.find(key);
    if (iter == mSounds.end())
        throw wren::Exception("invalid sound '{}'", key);

    SoundEffect& sound = iter->second;

    if (sound.handle == nullptr)
        throw wren::Exception("could not load sound '{}'", sound.get_key());

    sound.id = world.audio.play_sound(sound.handle.get(), sq::SoundGroup::Sfx, sound.volume, false);
}

void Fighter::wren_cancel_sound(SmallString key)
{
    const auto iter = mSounds.find(key);
    if (iter == mSounds.end())
        throw wren::Exception("invalid sound '{}'", key);

    if (iter->second.id != -1)
        world.audio.stop_sound(iter->second.id);
}

//============================================================================//

void FighterAction::wren_log_with_prefix(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {} Action {:<19}| {}"_format(fighter.index, name, message));
}

void FighterAction::wren_cxx_before_start()
{
//    if (world.options.log_script == true)
//        wren_log_with_prefix("start");

    mCurrentFrame = mWaitUntil = 0u;

    mHitSomething = false;

    // don't need to set to null because do_start will assign a new fiber anyway
    if (mFiberHandle) wrenReleaseHandle(world.vm, mFiberHandle);
}

void FighterAction::wren_cxx_wait_until(uint frame)
{
    if (frame < mCurrentFrame)
        throw wren::Exception("frame {} has already happened", frame);

    mWaitUntil = frame;
}

void FighterAction::wren_cxx_wait_for(uint frames)
{
    if (frames == 0u)
        throw wren::Exception("can't wait for zero frames");

    mWaitUntil = mCurrentFrame + frames;
}

bool FighterAction::wren_cxx_next_frame()
{
    return ++mCurrentFrame > mWaitUntil;
}

void FighterAction::wren_cxx_before_cancel()
{
//    if (world.options.log_script == true)
//        wren_log_with_prefix("cancel");
}

//----------------------------------------------------------------------------//

void FighterAction::wren_enable_hitblobs(StringView prefix)
{
    if (prefix.length() > TinyString::capacity())
        throw wren::Exception("hitblob prefix too long");

    bool found = false;

    for (auto& [key, blob] : mBlobs)
        if (key.starts_with(prefix) == true)
            world.enable_hitblob(&blob), found = true;

    if (found == false)
        throw wren::Exception("no hitblobs matching '{}*'", prefix);
}

void FighterAction::wren_disable_hitblobs(bool resetCollisions)
{
    world.disable_hitblobs(*this);

    if (resetCollisions == true)
        world.reset_collisions(fighter.index);
}

void FighterAction::wren_play_effect(TinyString key)
{
    const auto iter = mEffects.find(key);
    if (iter == mEffects.end())
        throw wren::Exception("invalid effect '{}'", key);

    VisualEffect& effect = iter->second;

    if (effect.handle == nullptr)
        throw wren::Exception("could not load effect '{}'", effect.get_key());

    world.get_effect_system().play_effect(effect);
}

void FighterAction::wren_emit_particles(TinyString key)
{
    const auto iter = mEmitters.find(key);
    if (iter == mEmitters.end())
        throw wren::Exception("invalid emitter '{}'", key);

    Emitter& emitter = iter->second;

    world.get_particle_system().generate(emitter);
}

//============================================================================//

void FighterState::wren_log_with_prefix(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {} State  {:<19}| {}"_format(fighter.index, name, message));
}

void FighterState::wren_cxx_before_enter()
{
//    if (world.options.log_script == true)
//        wren_log_with_prefix("enter");
}

void FighterState::wren_cxx_before_exit()
{
//    if (world.options.log_script == true)
//        wren_log_with_prefix("exit");
}

//============================================================================//

double World::wren_random_int(int min, int max)
{
    return double(std::uniform_int_distribution(min, max)(mRandNumGen));
}

double World::wren_random_float(float min, float max)
{
    return double(std::uniform_real_distribution(min, max)(mRandNumGen));
}
