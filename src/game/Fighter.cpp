#include "game/Fighter.hpp"

#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/World.hpp"

#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

Fighter::Fighter(const FighterDef& def, uint8_t index)
    : Entity(def), def(def), index(index)
{
    initialise_attributes();
    initialise_armature();
    initialise_hurtblobs();
    initialise_actions();
    initialise_states();

    // create fighter library
    {
        const String module = "{}/Library"_format(def.directory);
        world.vm.load_module(module.c_str());
        mLibraryHandle = world.vm.call<WrenHandle*> (
            world.handles.new_1, wren::GetVar(module.c_str(), "Library"), this
        );
    }

    // todo: proper action for entry upon game start
    activeState = &mStates.at("Neutral");
    activeState->call_do_enter();
    play_animation(def.animations.at("NeutralLoop"), 0u, true);
}

Fighter::~Fighter()
{
    if (mLibraryHandle) wrenReleaseHandle(world.vm, mLibraryHandle);
}

//============================================================================//

void Fighter::initialise_armature()
{
    mFadeStartSample.resize(def.armature.get_rest_sample().size());
}

//============================================================================//

void Fighter::initialise_attributes()
{
    attributes = def.attributes;

    localDiamond.halfWidth = attributes.diamondHalfWidth;
    localDiamond.offsetCross = attributes.diamondOffsetCross;
    localDiamond.offsetTop = attributes.diamondOffsetTop;

    localDiamond.compute_normals();
}

//============================================================================//

void Fighter::initialise_hurtblobs()
{
    mHurtBlobs.reserve(def.hurtBlobs.size());

    for (const auto& [key, def] : def.hurtBlobs)
        mHurtBlobs.emplace_back(def, *this);
}

//============================================================================//

void Fighter::initialise_actions()
{
    for (const auto& [key, def] : def.actions)
        mActions.try_emplace(mActions.end(), key, def, *this);
}

//============================================================================//

void Fighter::initialise_states()
{
    for (const auto& [key, def] : def.states)
        mStates.try_emplace(mStates.end(), key, def, *this);
}

//============================================================================//

void Fighter::accumulate_hit(const HitBlob& hit, const HurtBlob& hurt)
{
    SQASSERT(hit.entity != this, "invalid hitblob");
    SQASSERT(&hurt.fighter == this, "invalid hurtblob");

    const Attributes& attrs = attributes;
    Variables& vars = variables;
    EntityVars& otherVars = hit.entity->get_vars();

    // if we were in the middle of an action, cancel it
    cancel_action();

    //--------------------------------------------------------//

    const bool shielding = activeState->def.name == "Shield" || activeState->def.name == "ShieldStun";

    // https://www.ssbwiki.com/Hitlag#Formula
    const uint8_t freezeTime = [&]() {
        const float shield = shielding ? 0.75f : 1.f;
        const float time = (hit.def.damage * 0.5f + 4.f) * hit.def.freezeMult * shield;
        return uint8_t(std::min(time, 32.f));
    }();

    // https://www.ssbwiki.com/Angle#Angle_flipper
    const float facing = [&]() {
        if (hit.def.facingMode == BlobFacingMode::Forward) return +float(otherVars.facing);
        if (hit.def.facingMode == BlobFacingMode::Reverse) return -float(otherVars.facing);
        return otherVars.position.x < vars.position.x ? +1.f : -1.f; // Relative
    }();

    //--------------------------------------------------------//

    vars.freezeTime = std::max(vars.freezeTime, freezeTime);

    otherVars.freezeTime = std::max(otherVars.freezeTime, freezeTime);

    // todo: add self to a list of somethings
    otherVars.hitSomething = true;

    hit.entity->wren_play_sound(hit.def.sound, false);

    //--------------------------------------------------------//

    if (shielding == true)
    {
        // commutative, don't need to defer anything to finish_apply_hits()

        if ((vars.shield -= hit.def.damage) > 0.f)
        {
            vars.velocity.x += facing * (SHIELD_PUSH_HURT_BASE + hit.def.damage * SHIELD_PUSH_HURT_FACTOR);

            if (dynamic_cast<Fighter*>(hit.entity) != nullptr)
                otherVars.velocity.x -= facing * (SHIELD_PUSH_HIT_BASE + hit.def.damage * SHIELD_PUSH_HIT_FACTOR);

            const float stunTime = SHIELD_STUN_BASE + SHIELD_STUN_FACTOR * hit.def.damage;
            vars.stunTime = std::max(vars.stunTime, uint8_t(std::min(stunTime, 255.f)));

            change_state(mStates.at("ShieldStun"));
        }
        // todo: make ShieldBreak a real action
        //else start_action("ShieldBreak");

        // todo: play shield hit sound

        return;
    }

    //--------------------------------------------------------//

    vars.damage += hit.def.damage;

    // https://www.ssbwiki.com/Knockback#Melee_onward

    // todo: for ignore weight, set gravity and fall speed as in https://www.ssbwiki.com/Knockback#Ultimate

    const float knockback = [&]() {
        const float b = hit.def.ignoreDamage ? 0.f : hit.def.knockBase;
        const float d = hit.def.ignoreDamage ? hit.def.knockBase : hit.def.damage;
        const float p = hit.def.ignoreDamage ? 10.f : vars.damage;
        const float w = hit.def.ignoreWeight ? 100.f : attrs.weight;
        const float s = hit.def.knockScale;
        return (((p / 10.f + p * d / 20.f) * 200.f / (w + 100.f) * 1.4f + 18.f) * s * 0.01f + b);
    }();

    // todo:
    // investigate changing angles to signed values in the range [-180, +180]
    // possibly even [-90, +90], do any attacks use relative facing with reverse angles?
    const float angle = [&]() {
        // https://www.ssbwiki.com/Sakurai_angle
        if (hit.def.angleMode == BlobAngleMode::Sakurai)
        {
            if (vars.onGround == false) return maths::radians(45.f / 360.f);
            if (knockback < 60.f) return 0.f;
            const float blend = std::clamp((knockback - 60.f) / 30.f, 0.f, 1.f);
            return maths::radians(maths::mix(10.f, 40.f, blend) / 360.f);
        }
        // https://www.ssbwiki.com/Autolink_angle
        if (hit.def.angleMode == BlobAngleMode::AutoLink)
        {
            // todo
            return maths::radians(hit.def.knockAngle / 360.f);
        }
        return maths::radians(hit.def.knockAngle / 360.f);
    }();

    enum class LaunchMode { Replace, Ignore, Merge };

    const LaunchMode launchMode = [&]() {
        // subsequent hits from the same entity
        if (vars.launchEntity == hit.entity->eid) return LaunchMode::Replace;
        // new speed is greater than old speed
        if (knockback * 0.003f > vars.launchSpeed) return LaunchMode::Replace;
        // new speed is much less than old speed
        if (knockback * 0.003f * 2.f < vars.launchSpeed) return LaunchMode::Ignore;
        // new speed is equal to or slightly less than old speed
        return LaunchMode::Merge;
    }();

    if (launchMode == LaunchMode::Replace)
    {
        vars.stunTime = uint8_t(std::min(knockback * 0.4f, 255.f));

        vars.launchSpeed = knockback * 0.003f;
        vars.velocity = Vec2F(std::cos(angle) * facing, std::sin(angle)) * vars.launchSpeed;

        vars.launchEntity = hit.entity->eid;
    }
    else // ignore or merge
    {
        const uint8_t newStunTime = uint8_t(std::min(knockback * 0.4f, 255.f));
        const uint8_t maxStunTime = uint8_t(vars.launchSpeed / KNOCKBACK_DECAY);

        // increase hitstun if possible
        vars.stunTime = std::max(vars.stunTime, std::min(newStunTime, maxStunTime));

        if (launchMode == LaunchMode::Merge)
        {
            // todo: this is different to how smash works and not well tested

            const Vec2F newVelocity = Vec2F(std::cos(angle) * facing, std::sin(angle)) * knockback * 0.003f;
            const Vec2F mergeVelocity = vars.velocity + newVelocity;

            const float mvLengthSquared = maths::length_squared(mergeVelocity);

            if (mvLengthSquared > 0.00001f)
                vars.velocity = (mergeVelocity / std::sqrt(mvLengthSquared)) * vars.launchSpeed;

            // opposite directions, go straight up
            else vars.velocity = Vec2F(0.f, vars.launchSpeed);

            vars.launchEntity = -1;
        }
    }

    sq::log_debug_multiline("fighter {} hit by entity {}:"
                            "\nknockback:   {}" "\nangle:       {}" "\nvelocity:    {}"
                            "\nfreezeTime:  {}" "\nstunTime:    {}" "\ndecayFrames: {}"
                            "\nhitKey:      {}" "\nhurtKey:     {}" "\nhurtRegion:  {}",
                            index, hit.entity->eid,
                            knockback, angle, vars.velocity,
                            vars.freezeTime, vars.stunTime, uint(vars.launchSpeed / KNOCKBACK_DECAY),
                            hit.def.get_key(), hurt.def.get_key(), hurt.def.region);

    SQASSERT(vars.stunTime < uint(vars.launchSpeed / KNOCKBACK_DECAY), "something is broken");

    mHurtRegion = !mHurtRegion.has_value() ? hurt.def.region :
                  hurt.def.region != *mHurtRegion ? BlobRegion::Middle :
                  mHurtRegion;
}

//============================================================================//

void Fighter::apply_hits()
{
    SQASSERT(mHurtRegion.has_value(), "not hit by anything");

    Variables& vars = variables;

    // rotate towards the camera over the duration of hitstun
    if (vars.velocity.x != 0.f)
    {
        // opposite of launch direction
        const int8_t newFacing = std::signbit(vars.velocity.x) ? +1 : -1;
        if (vars.facing != newFacing)
            wren_reverse_facing_slow(vars.facing == 1, vars.stunTime);
    }

    // always start jittering with an even index
    mJitterCounter = mJitterCounter & 254u;

    const bool heavy = vars.stunTime >= MIN_HITSTUN_HEAVY;

    TinyString state;
    SmallString animNow, animAfter;

    const auto set_state_and_anims_fall = [&]()
    {
        state = "FallStun";
        animNow = heavy ? "HurtAirHeavy" : "HurtAirLight";
        animAfter = "FallLoop";
    };

    const auto set_state_and_anims_neutral = [&]()
    {
        state = "NeutralStun";
        if (*mHurtRegion == BlobRegion::Middle) animNow = heavy ? "HurtMiddleHeavy" : "HurtMiddleLight";
        else if (*mHurtRegion == BlobRegion::Lower) animNow = heavy ? "HurtLowerHeavy" : "HurtLowerLight";
        else if (*mHurtRegion == BlobRegion::Upper) animNow = heavy ? "HurtUpperHeavy" : "HurtUpperLight";
        else SQEE_UNREACHABLE();
        animAfter = "NeutralLoop";

    };

    const auto set_state_and_anims_tumble = [&]()
    {
        state = "TumbleStun";
        if (*mHurtRegion == BlobRegion::Middle) animNow = "HurtMiddleTumble";
        else if (*mHurtRegion == BlobRegion::Lower) animNow = "HurtLowerTumble";
        else if (*mHurtRegion == BlobRegion::Upper) animNow = "HurtUpperTumble";
        else SQEE_UNREACHABLE();
        animAfter = "TumbleLoop";
    };

    // todo: need a new state and animation
    //const auto set_state_and_anims_prone = [&]()
    //{
    //    state = "ProneStun";
    //    animNow = "HurtProne";
    //    animAfter = "ProneLoop";
    //};

    // in the air or launched upwards
    if (vars.onGround == false || vars.velocity.y > 0.f)
    {
        if (vars.stunTime < MIN_HITSTUN_TUMBLE) set_state_and_anims_fall();
        else set_state_and_anims_tumble();
    }

    // on the ground and launched downwards
    else if (vars.velocity.y < 0.f)
    {
        if (vars.stunTime < MIN_HITSTUN_TUMBLE) set_state_and_anims_neutral();
        else set_state_and_anims_tumble();
    }

    // on the ground and launched horizontally
    else set_state_and_anims_neutral();

    change_state(mStates.at(state));
    play_animation(def.animations.at(animNow), 0u, true);
    set_next_animation(def.animations.at(animAfter), 0u);

    update_animation();

    mModelMatrix = maths::transform(current.translation, current.rotation);

    mHurtRegion = std::nullopt;
}

//============================================================================//

void Fighter::apply_rebound(float damage)
{
    Variables& vars = variables;

    // https://www.ssbwiki.com/Priority#Rebound
    cancel_action();
    // todo: allow slowing down animation for really high damage attacks
    vars.reboundTime = uint8_t(std::min(damage + 7.f, 31.f));
    start_action(mActions.at("Rebound"));
}

//============================================================================//

void Fighter::pass_boundary()
{
    // todo: this should be its own action
    reset_everything();
    activeState = &mStates.at("Neutral");
    activeState->call_do_enter();
    play_animation(def.animations.at("NeutralLoop"), 0u, true);
}

//============================================================================//

void Fighter::start_action(FighterAction& action)
{
    SQASSERT(activeAction == nullptr, "call clear_action first");

    //if (&action != editorErrorCause)
    {
        activeAction = &action;
        activeAction->call_do_start();
    }
}

void Fighter::cancel_action()
{
    if (activeAction == nullptr) return;

    //if (activeAction != editorErrorCause)
        activeAction->call_do_cancel();

    clear_action();
}

void Fighter::change_state(FighterState& state)
{
    // todo: deal with errors from states

    // will be null if the fighter was just created or reset
    if (activeState != nullptr)
        activeState->call_do_exit();

    activeState = &state;
    activeState->call_do_enter();
}

//============================================================================//

void Fighter::clear_action()
{
    for (int32_t id : mTransientEffects)
        world.wren_cancel_effect(id);

    for (int32_t id : mTransientSounds)
        world.wren_cancel_sound(id);

    mTransientEffects.clear();
    mTransientSounds.clear();

    activeAction = nullptr;
}

//============================================================================//

void Fighter::reset_everything()
{
    // cancel action and exit from state
    if (activeAction) { activeAction->call_do_cancel(); activeAction = nullptr; }
    if (activeState) { activeState->call_do_exit(); activeState = nullptr; }

    // reset variables and interpolation data
    variables = Variables();

    previous.translation = current.translation = Vec3F();
    previous.rotation = current.rotation = QuatF();
    mAnimPlayer.previousSample = def.armature.get_rest_sample();
    mAnimPlayer.currentSample = def.armature.get_rest_sample();

    // reset internal data
    mRootMotionPreviousOffset = Vec3F();
    mRootMotionTranslate = Vec2F();
    mRotateMode = RotateMode::Auto;

    // reset editor data
    editorStartAction = nullptr;
}
