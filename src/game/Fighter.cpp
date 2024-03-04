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
        const String module = def.directory + "/Library";
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

bool Fighter::accumulate_hit(const HitBlob& hit, const HurtBlob& hurt)
{
    SQASSERT(hit.entity != this && hit.def.type == BlobType::Damage, "invalid hitblob");
    SQASSERT(&hurt.fighter == this, "invalid hurtblob");

    Variables& vars = variables;
    EntityVars& otherVars = hit.entity->get_vars();

    // todo: add self to a list of somethings
    otherVars.hitSomething = true;

    if (!hit.def.sound.empty())
        hit.entity->wren_play_sound(hit.def.sound, false);

    const bool shielding = activeState->def.name == "Shield" || activeState->def.name == "ShieldStun";

    //--------------------------------------------------------//

    // https://www.ssbwiki.com/Hitlag#Formula
    const uint8_t freezeTime = [&]() {
        if (world.editor != nullptr) return uint8_t(0); // prevent desync in editor
        const float shield = shielding ? 0.75f : 1.f;
        const float time = (hit.def.damage * 0.5f + 4.f) * hit.def.freezeMult * shield;
        return uint8_t(std::min(time, 32.f));
    }();

    otherVars.freezeTime = std::max(otherVars.freezeTime, freezeTime);

    // if we are invincible, only the attacker gets freeze time
    if (vars.invincible || hurt.invincible)
        return false;

    vars.freezeTime = std::max(vars.freezeTime, freezeTime);

    //--------------------------------------------------------//

    if (shielding == true)
    {
        const float dir = otherVars.position.x < vars.position.x ? +1.f : -1.f;

        vars.shield -= hit.def.damage;
        vars.velocity.x += dir * (SHIELD_PUSH_HURT_BASE + hit.def.damage * SHIELD_PUSH_HURT_FACTOR);

        if (dynamic_cast<Fighter*>(hit.entity) != nullptr)
            otherVars.velocity.x -= dir * (SHIELD_PUSH_HIT_BASE + hit.def.damage * SHIELD_PUSH_HIT_FACTOR);

        const float stunTime = SHIELD_STUN_BASE + SHIELD_STUN_FACTOR * hit.def.damage;
        vars.stunTime = std::max(vars.stunTime, uint8_t(std::min(stunTime, 255.f)));

        return true;
    }

    //--------------------------------------------------------//

    // if we were in the middle of an action, cancel it
    cancel_action();

    vars.damage += hit.def.damage;

    // when grabbed, prevent knockback from our bully or from weak attacks
    if (vars.bully != nullptr)
        if (vars.bully == hit.entity || hit.def.damage < 10.f)
            return true;

    //--------------------------------------------------------//

    sq::log_debug("fighter {} hit by entity {}", index, hit.entity->eid);

    apply_knockback(hit.def, *hit.entity);

    SQASSERT(vars.stunTime < uint(vars.launchSpeed / KNOCKBACK_DECAY), "something is broken");

    mHurtRegion = !mHurtRegion.has_value() ? hurt.def.region :
                  hurt.def.region != *mHurtRegion ? BlobRegion::Middle :
                  mHurtRegion;

    return true;
}

//============================================================================//

void Fighter::apply_knockback(const HitBlobDef& hitDef, const Entity& hitEntity)
{
    SQASSERT(&hitEntity != this && hitDef.type == BlobType::Damage, "invalid hitblob");

    const Attributes& attrs = attributes;
    Variables& vars = variables;
    const EntityVars& otherVars = hitEntity.get_vars();

    // https://www.ssbwiki.com/Knockback#Melee_onward

    // todo: for ignore weight, set gravity and fall speed as in https://www.ssbwiki.com/Knockback#Ultimate

    const float knockback = [&]() {
        const float b = hitDef.ignoreDamage ? 0.f : hitDef.knockBase;
        const float d = hitDef.ignoreDamage ? hitDef.knockBase : hitDef.damage;
        const float p = hitDef.ignoreDamage ? 10.f : vars.damage;
        const float w = hitDef.ignoreWeight ? 100.f : attrs.weight;
        const float s = hitDef.knockScale;
        return (((p / 10.f + p * d / 20.f) * 200.f / (w + 100.f) * 1.4f + 18.f) * s * 0.01f + b);
    }();

    // todo:
    // investigate changing angles to signed values in the range [-180, +180]
    // possibly even [-90, +90], do any attacks use relative facing with reverse angles?
    const float angle = [&]() {
        // https://www.ssbwiki.com/Sakurai_angle
        if (hitDef.angleMode == BlobAngleMode::Sakurai)
        {
            if (vars.onGround == false) return maths::radians(45.f / 360.f);
            if (knockback < 60.f) return 0.f;
            const float blend = std::clamp((knockback - 60.f) / 30.f, 0.f, 1.f);
            return maths::radians(maths::mix(10.f, 40.f, blend) / 360.f);
        }
        // https://www.ssbwiki.com/Autolink_angle
        if (hitDef.angleMode == BlobAngleMode::AutoLink)
        {
            // todo
            return maths::radians(hitDef.knockAngle / 360.f);
        }
        return maths::radians(hitDef.knockAngle / 360.f);
    }();

    // https://www.ssbwiki.com/Angle#Angle_flipper
    const float facing = [&]() {
        if (hitDef.facingMode == BlobFacingMode::Forward) return +float(otherVars.facing);
        if (hitDef.facingMode == BlobFacingMode::Reverse) return -float(otherVars.facing);
        return otherVars.position.x < vars.position.x ? +1.f : -1.f; // Relative
    }();

    enum class LaunchMode { Replace, Ignore, Merge };

    const LaunchMode launchMode = [&]() {
        // subsequent hits from the same entity
        if (vars.launchEntity == hitEntity.eid) return LaunchMode::Replace;
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

        vars.launchEntity = hitEntity.eid;
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
}

//============================================================================//

void Fighter::apply_hits()
{
    Variables& vars = variables;

    if (activeState->def.name == "Shield" || activeState->def.name == "ShieldStun")
    {
        if (vars.shield <= 0.f)
        {
            // todo
            //start_action("ShieldBreak");
        }
        else
        {
            // todo: play shield hit sound
            change_state(mStates.at("ShieldStun"));
        }

        return;
    }

    // no hits did enough damage to break the grab
    if (vars.bully != nullptr && mHurtRegion.has_value() == false)
    {
        play_animation(def.animations.at("GrabbedHurtLow"), 0u, true);
        set_next_animation(def.animations.at("GrabbedLoopLow"), 0u);

        update_animation();
        mModelMatrix = maths::transform(current.translation, current.rotation);

        return;
    }

    // finish any in-progress rotation
    current.rotation = QuatF(0.f, 0.25f * float(vars.facing), 0.f);

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

void Fighter::apply_grab(Fighter& victim)
{
    sq::log_debug("fighter {} grabbed by fighter {}", victim.index, index);
    mHitBlobs.clear();

    cancel_action();
    victim.cancel_action();

    variables.victim = &victim;
    victim.variables.bully = this;

    start_action(mActions.at("GrabStart"));
    victim.start_action(victim.mActions.at("GrabbedStart"));

    update_animation();
    mModelMatrix = maths::transform(current.translation, current.rotation);

    victim.update_animation();
    victim.mModelMatrix = maths::transform(victim.current.translation, victim.current.rotation);
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
    change_state(mStates.at("Neutral"));
    play_animation(def.animations.at("NeutralLoop"), 0u, true);
}

//============================================================================//

void Fighter::set_position_after_throw()
{
    // Set real position relative to the current position of the Rot bone. This
    // method should only be called during our bully's update.
    //
    // Note that this method does not give the same results as brawl. Don't
    // really care for now as this gives better visual results anyway.

    const sq::Armature::Bone* restSampleBones =
        reinterpret_cast<const sq::Armature::Bone*>(def.armature.get_rest_sample().data());

    sq::Armature::Bone* currentSampleBones =
        reinterpret_cast<sq::Armature::Bone*>(mAnimPlayer.currentSample.data());

    const Mat4F matrix = mModelMatrix * def.armature.compute_bone_matrix(mAnimPlayer.currentSample, 1u);

    variables.position.y = matrix[3].y - restSampleBones[1].offset.z;
    variables.position.y = std::max(variables.position.y, variables.bully->get_vars().position.y);

    // adjust current pose so that it can be faded from

    current.translation.y = variables.position.y;

    const Mat4F modelMat = maths::transform(current.translation, current.rotation);
    const Mat4F transBoneMat = Mat4F(QuatF(0.f, 0.70710677f, 0.70710677f, 0.f));

    currentSampleBones[1].offset = Vec3F(maths::inverse(modelMat * transBoneMat) * matrix[3]);
}

//============================================================================//

void Fighter::set_spawn_transform(Vec2F position, int8_t facing)
{
    variables.position = position;

    diamond.cross.x = position.x;
    diamond.min.x = position.x - attributes.diamondMinWidth * 0.5f;
    diamond.max.x = position.x + attributes.diamondMinWidth * 0.5f;

    diamond.cross.y = position.y + attributes.diamondMinHeight * 0.5f;
    diamond.min.y = position.y;
    diamond.max.y = position.y + attributes.diamondMinHeight;

    variables.facing = facing;

    current.translation = Vec3F(position, 0.f);
    current.rotation = QuatF(0.f, 0.25f * float(facing), 0.f);
    mModelMatrix = maths::transform(current.translation, current.rotation);

    // might have problems later, but for now looks better than t-posing
    std::memset(mAnimPlayer.currentSample.data(), 0, mAnimPlayer.currentSample.size());
}

//============================================================================//

Diamond Fighter::compute_diamond() const
{
    // note that this uses the pose from the previous frame, which is fine, but does cause some slight stretching

    const Attributes& attrs = attributes;
    const Variables& vars = variables;

    Vec2F realMin = Vec2F(+INFINITY);
    Vec2F realMax = Vec2F(-INFINITY);

    for (uint8_t bone : attrs.diamondBones)
    {
        // todo: this is doing a lot of duplicate work, bone matrices need a cache
        const Mat4F matrix = mModelMatrix * def.armature.compute_bone_matrix(mAnimPlayer.currentSample, bone);
        realMin = maths::min(realMin, Vec2F(matrix[3]));
        realMax = maths::max(realMax, Vec2F(matrix[3]));
    }

    Diamond result;

    result.cross.x = vars.position.x;
    result.min.x = maths::min(realMin.x, vars.position.x - attrs.diamondMinWidth * 0.5f);
    result.max.x = maths::max(realMax.x, vars.position.x + attrs.diamondMinWidth * 0.5f);

    result.cross.y = maths::max((realMin.y + realMax.y) * 0.5f, vars.position.y + attrs.diamondMinHeight * 0.5f);
    result.min.y = vars.position.y;
    result.max.y = maths::max(realMax.y, result.cross.y + attrs.diamondMinHeight * 0.5f);

    return result;
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

    // reset variables
    variables = Variables();

    // disable active hitblobs
    mHitBlobs.clear();

    // reset internal data
    mRootMotionPreviousOffset = Vec3F();
    mRootMotionTranslate = Vec2F();
    mRotateMode = RotateMode::Auto;

    // reset editor data
    editorStartAction = nullptr;
    editorApplyGrab = nullptr;

    // reset transforms and pose
    set_spawn_transform({0.f, 0.f}, +1);
}
