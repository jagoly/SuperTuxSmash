#include "game/Article.hpp"
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

using namespace sts;

// todo: move disable_hitblobs from Article/Fighter to Entity

//============================================================================//

void Entity::wren_reverse_facing_auto()
{
    EntityVars& vars = get_vars();

    vars.facing = -vars.facing;

    mRotateMode = RotateMode::Auto;

    if (auto fighter = dynamic_cast<Fighter*>(this))
        for (InputFrame& frame : fighter->controller->history.frames)
            frame.set_relative_x(vars.facing);
}

void Entity::wren_reverse_facing_instant()
{
    wren_reverse_facing_auto();

    previous.rotation = QuatF(0.f, 0.25f * float(get_vars().facing), 0.f);
    current.rotation = previous.rotation;
}

void Entity::wren_reverse_facing_slow(bool clockwise, uint8_t time)
{
    wren_reverse_facing_auto();

    if (clockwise == false) mRotateMode = RotateMode::Slow;
    else mRotateMode = RotateMode::Slow | RotateMode::Clockwise;

    mRotateSlowTime = time;
    mRotateSlowProgress = 0u;
}

void Entity::wren_reverse_facing_animated(bool clockwise)
{
    wren_reverse_facing_auto();

    if (clockwise == false) mRotateMode = RotateMode::Animation;
    else mRotateMode = RotateMode::Animation | RotateMode::Clockwise;

    mFadeStartRotation = QuatF(0.f, -0.25f * float(get_vars().facing), 0.f);
}

void Entity::wren_reset_collisions()
{
    mIgnoreCollisions.clear();
}

void Entity::wren_play_animation(SmallString key, uint fade, bool fromStart)
{
    const EntityDef& def = get_def();

    const auto iter = def.animations.find(key);
    if (iter == def.animations.end())
        throw wren::Exception("invalid animation '{}'", key);

    if (fromStart == false && iter->second.anim.frameCount <= mAnimPlayer.animTime)
        throw wren::Exception("current frame is {}, but animation only has {}", mAnimPlayer.animTime, iter->second.anim.frameCount);

    play_animation(iter->second, fade, fromStart);
}

void Entity::wren_set_next_animation(SmallString key, uint fade)
{
    const EntityDef& def = get_def();

    const auto iter = def.animations.find(key);
    if (iter == def.animations.end())
        throw wren::Exception("invalid animation '{}'", key);

    if (mAnimPlayer.animation != nullptr && (mAnimPlayer.animation->manual == true || mAnimPlayer.animation->loop == true))
        throw wren::Exception("already playing a loop animation, '{}'", mAnimPlayer.animation->get_key());

    if (mAnimPlayer.animation == &iter->second)
        throw wren::Exception("specified animation already playing");

    set_next_animation(iter->second, fade);
}

int32_t Entity::wren_play_sound(SmallString key, bool stopWithAction)
{
    const EntityDef& def = get_def();

    const auto iter = def.sounds.find(key);
    if (iter == def.sounds.end())
        throw wren::Exception("invalid sound '{}'", key);

    const SoundEffect& sound = iter->second;

    if (sound.handle == nullptr)
        throw wren::Exception("could not load sound '{}'", sound.get_key());

    int32_t id = world.audio.play_sound(sound.handle.get(), sq::SoundGroup::Sfx, sound.volume, false);

    if (stopWithAction == true)
    {
        if (auto fighter = dynamic_cast<Fighter*>(this))
            if (fighter->activeAction == nullptr)
                throw wren::Exception("no active action");

        if (mTransientSounds.full() == true)
            throw wren::Exception("too many transient sounds");

        mTransientSounds.push_back(id);
    }

    return id;
}

//----------------------------------------------------------------------------//

void Entity::impl_wren_enable_hitblobs(const std::map<TinyString, HitBlobDef>& blobs, StringView prefix)
{
    if (prefix.length() > TinyString::capacity())
        throw wren::Exception("hitblob prefix too long");

    bool found = false;

    for (auto& [key, def] : blobs)
        if (key.starts_with(prefix) == true)
            mHitBlobs.emplace_back(def, this),
            found = true;

    if (found == false)
        throw wren::Exception("no hitblobs matching '{}*'", prefix);
}

int32_t Entity::impl_wren_play_effect(const std::map<TinyString, VisualEffectDef>& effects, TinyString key)
{
    const auto iter = effects.find(key);
    if (iter == effects.end())
        throw wren::Exception("invalid effect '{}'", key);

    const VisualEffectDef& effect = iter->second;

    if (effect.handle == nullptr)
        throw wren::Exception("could not load effect '{}'", effect.get_key());

    int32_t id = world.get_effect_system().play_effect(effect, this);

    if (effect.transient == true)
    {
        if (auto fighter = dynamic_cast<Fighter*>(this))
            if (fighter->activeAction == nullptr)
                throw wren::Exception("no active action");

        if (mTransientEffects.full() == true)
            throw wren::Exception("too many transient effects");

        mTransientEffects.push_back(id);
    }

    return id;
}

void Entity::impl_wren_emit_particles(const std::map<TinyString, Emitter>& emitters, TinyString key)
{
    const auto iter = emitters.find(key);
    if (iter == emitters.end())
        throw wren::Exception("invalid emitter '{}'", key);

    const Emitter& emitter = iter->second;

    world.get_particle_system().generate(emitter, this);
}

//============================================================================//

void Article::wren_log_with_prefix(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {} Article {:<18}| {}", fighter->index, eid, message);
}

void Article::wren_cxx_wait_until(uint frame)
{
    if (frame < mCurrentFrame)
        throw wren::Exception("frame {} has already happened", frame);

    mWaitUntil = frame;
}

void Article::wren_cxx_wait_for(uint frames)
{
    if (frames == 0u)
        throw wren::Exception("can't wait for zero frames");

    mWaitUntil = mCurrentFrame + frames;
}

bool Article::wren_cxx_next_frame()
{
    return ++mCurrentFrame > mWaitUntil;
}

//----------------------------------------------------------------------------//

void Article::wren_enable_hitblobs(StringView prefix)
{
    impl_wren_enable_hitblobs(def.blobs, prefix);
}

void Article::wren_disable_hitblobs(bool resetCollisions)
{
    mHitBlobs.clear();

    if (resetCollisions == true)
        mIgnoreCollisions.clear();
}

int32_t Article::wren_play_effect(TinyString key)
{
    return impl_wren_play_effect(def.effects, key);
}

void Article::wren_emit_particles(TinyString key)
{
    return impl_wren_emit_particles(def.emitters, key);
}

//============================================================================//

void Fighter::wren_log(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {}: {}", index, message);
}

void Fighter::wren_cxx_assign_action(SmallString key)
{
    clear_action();

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

Article* Fighter::wren_cxx_spawn_article(TinyString key)
{
    const auto iter = def.articles.find(key);
    if (iter == def.articles.end())
        throw wren::Exception("invalid article '{}'", key);

    return &world.create_article(iter->second, this);
}

//----------------------------------------------------------------------------//

bool Fighter::wren_attempt_ledge_catch()
{
    Variables& vars = variables;

    if (vars.ledge != nullptr)
        throw wren::Exception("already have a ledge");

    if (vars.noCatchTime != 0u || vars.velocity.y > 0.f)
        return false;

    vars.ledge = world.get_stage().find_ledge (
        localDiamond, vars.position, vars.facing, controller->history.frames.front().intX
    );

    return vars.ledge != nullptr;
}

void Fighter::wren_enable_hurtblob(TinyString key)
{
    const auto iter = ranges::find_if(mHurtBlobs, [&](const HurtBlob& blob) { return blob.def.get_key() == key; });
    if (iter == mHurtBlobs.end())
        throw wren::Exception("invalid hurt blob '{}'", key);

    iter->intangible = false;
}

void Fighter::wren_disable_hurtblob(TinyString key)
{
    const auto iter = ranges::find_if(mHurtBlobs, [&](const HurtBlob& blob) { return blob.def.get_key() == key; });
    if (iter == mHurtBlobs.end())
        throw wren::Exception("invalid hurt blob '{}'", key);

    iter->intangible = true;
}

//============================================================================//

void FighterAction::wren_log_with_prefix(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {} Action {:<19}| {}", fighter.index, def.name, message);
}

void FighterAction::wren_cxx_before_start()
{
//    if (world.options.log_script == true)
//        wren_log_with_prefix("start");

    mCurrentFrame = mWaitUntil = 0u;

    fighter.variables.hitSomething = false;

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
    fighter.impl_wren_enable_hitblobs(def.blobs, prefix);
}

void FighterAction::wren_disable_hitblobs(bool resetCollisions)
{
    fighter.get_hit_blobs().clear();

    if (resetCollisions == true)
        fighter.get_ignore_collisions().clear();
}

int32_t FighterAction::wren_play_effect(TinyString key)
{
    return fighter.impl_wren_play_effect(def.effects, key);
}

void FighterAction::wren_emit_particles(TinyString key)
{
    return fighter.impl_wren_emit_particles(def.emitters, key);
}

//============================================================================//

void FighterState::wren_log_with_prefix(StringView message)
{
    if (world.options.log_script == true)
        sq::log_debug("Fighter {} State  {:<19}| {}", fighter.index, def.name, message);
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

void World::wren_cancel_sound(int32_t id)
{
    audio.stop_sound(id);
}

void World::wren_cancel_effect(int32_t id)
{
    mEffectSystem->cancel_effect(id);

}
