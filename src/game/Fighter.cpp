#include "game/Fighter.hpp"

#include "main/Options.hpp"

#include "game/Controller.hpp"
#include "game/FightWorld.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"

#include "render/Renderer.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

Fighter::Fighter(FightWorld& world, FighterEnum name, uint8_t index)
    : world(world), name(name), index(index)
{
    initialise_attributes();
    initialise_armature();
    initialise_hurtblobs();

    initialise_animations();
    initialise_actions();
    initialise_states();

    // uniform buffer and descriptor set
    {
        const auto& ctx = sq::VulkanContext::get();

        mSkellyUbo.initialise(64u + 48u + 48u * mArmature.get_bone_count(), vk::BufferUsageFlagBits::eUniformBuffer);
        mDescriptorSet = sq::vk_allocate_descriptor_set_swapper(ctx, world.renderer.setLayouts.object);

        sq::vk_update_descriptor_set_swapper (
            ctx, mDescriptorSet, sq::DescriptorUniformBuffer(0u, 0u, mSkellyUbo.get_descriptor_info())
        );
    }

    // create draw items
    {
        const String path = "assets/fighters/{}/Render.json"_format(name);
        world.renderer.create_draw_items (
            DrawItemDef::load_from_json(path, world.caches),
            mDescriptorSet,
            { {"flinch", &variables.flinch} }
        );
    }

    // create fighter library
    {
        const String module = "fighters/{}/Library"_format(name);
        world.vm.load_module(module.c_str());
        mLibraryHandle = world.vm.call<WrenHandle*> (
            world.handles.new_1, wren::GetVar(module.c_str(), "Library"), this
        );
    }

    // todo: proper action for entry upon game start
    activeState = &mStates.at("Neutral");
    activeState->call_do_enter();
    play_animation(mAnimations.at("NeutralLoop"), 0u, true);
}

Fighter::~Fighter()
{
    if (mLibraryHandle) wrenReleaseHandle(world.vm, mLibraryHandle);
}

//============================================================================//

void Fighter::initialise_armature()
{
    const String path = "assets/fighters/{}/Armature.json"_format(name);

    mArmature.load_from_file(path);
    current.pose = previous.pose = mArmature.get_rest_pose();
    mBoneMatrices.resize(mArmature.get_bone_count());
}

//============================================================================//

void Fighter::initialise_attributes()
{
    const String path = "assets/fighters/{}/Attributes.json"_format(name);
    const JsonValue json = sq::parse_json_from_file(path);

    json.at("walkSpeed")   .get_to(attributes.walkSpeed);
    json.at("dashSpeed")   .get_to(attributes.dashSpeed);
    json.at("airSpeed")    .get_to(attributes.airSpeed);
    json.at("traction")    .get_to(attributes.traction);
    json.at("airMobility") .get_to(attributes.airMobility);
    json.at("airFriction") .get_to(attributes.airFriction);

    json.at("hopHeight")     .get_to(attributes.hopHeight);
    json.at("jumpHeight")    .get_to(attributes.jumpHeight);
    json.at("airHopHeight")  .get_to(attributes.airHopHeight);
    json.at("gravity")       .get_to(attributes.gravity);
    json.at("fallSpeed")     .get_to(attributes.fallSpeed);
    json.at("fastFallSpeed") .get_to(attributes.fastFallSpeed);
    json.at("weight")        .get_to(attributes.weight);

    json.at("walkAnimStride") .get_to(attributes.walkAnimStride);
    json.at("dashAnimStride") .get_to(attributes.dashAnimStride);

    json.at("extraJumps")    .get_to(attributes.extraJumps);
    json.at("lightLandTime") .get_to(attributes.lightLandTime);

    json.at("diamondHalfWidth")   .get_to(localDiamond.halfWidth);
    json.at("diamondOffsetCross") .get_to(localDiamond.offsetCross);
    json.at("diamondOffsetTop")   .get_to(localDiamond.offsetTop);

    localDiamond.compute_normals();
}

//============================================================================//

void Fighter::initialise_hurtblobs()
{
    const String path = "assets/fighters/{}/HurtBlobs.json"_format(name);
    const JsonValue json = sq::parse_json_from_file(path);

    for (const auto& item : json.items())
    {
        HurtBlob& blob = mHurtBlobs[item.key()];
        blob.fighter = this;

        blob.from_json(item.value());

        world.enable_hurtblob(&blob);
    }
}

//============================================================================//

void Fighter::initialise_animations()
{
    const auto load_animation = [this](const String& key, const String& mode)
    {
        if (auto [iter, ok] = mAnimations.try_emplace(key); ok)
        {
            try {
                const String path = "assets/fighters/{}/anims/{}"_format(name, key);
                iter->second.anim = mArmature.load_animation_from_file(path);
                iter->second.mode = sq::enum_from_string<AnimMode>(mode);
            }
            catch (const std::exception& ex) {
                sq::log_warning("failed to load animation '{}/{}': {}", name, key, ex.what());
                iter->second.anim = mArmature.make_null_animation(1u);
                iter->second.mode = sq::enum_from_string_safe<AnimMode>(mode).value_or(AnimMode::Basic);
            }

            // todo: add the mode into the .sqa files
            // something simple like 'Config [some string]' in the Header
            // then the json mode will only be used for fallback animations

            // animation doesn't have any motion
            if (iter->second.anim.bones[0][0].track.size() == sizeof(Vec3F))
            {
                if (iter->second.mode == AnimMode::Motion) iter->second.mode = AnimMode::Basic;
                if (iter->second.mode == AnimMode::MotionTurn) iter->second.mode = AnimMode::Turn;
            }
        }
        else sq::log_warning("already loaded animation '{}/{}'", name, key);
    };

    for (const auto& entry : sq::parse_json_from_file("assets/FighterAnimations.json"))
        load_animation(entry.at(0).get_ref<const String&>(), entry.at(1).get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file("assets/fighters/{}/Animations.json"_format(name)))
        load_animation(entry.at(0).get_ref<const String&>(), entry.at(1).get_ref<const String&>());
}

//============================================================================//

void Fighter::initialise_actions()
{
    const auto load_action = [this](const String& key)
    {
        if (auto [iter, ok] = mActions.try_emplace(key, *this, key); ok)
        {
            try {
                iter->second.load_json_from_file();
                iter->second.load_wren_from_file();
            }
            catch (const std::exception& ex) {
                sq::log_warning("failed to load action '{}/{}': {}", name, key, ex.what());
            }
        }
        else sq::log_warning("already loaded action '{}/{}'", name, key);
    };

    for (const auto& entry : sq::parse_json_from_file("assets/FighterActions.json"))
        load_action(entry.get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file("assets/fighters/{}/Actions.json"_format(name)))
        load_action(entry.get_ref<const String&>());
}

//============================================================================//

void Fighter::initialise_states()
{
    const auto load_state = [this](const String& key)
    {
        if (auto [iter, ok] = mStates.try_emplace(key, *this, key); ok)
        {
            try {
                iter->second.load_wren_from_file();
            }
            catch (const std::exception& ex) {
                sq::log_warning("failed to load state '{}/{}': {}", name, key, ex.what());
            }
        }
        else sq::log_warning("already loaded state '{}/{}'", name, key);
    };

    for (const auto& entry : sq::parse_json_from_file("assets/FighterStates.json"))
        load_state(entry.get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file("assets/fighters/{}/States.json"_format(name)))
        load_state(entry.get_ref<const String&>());
}

//============================================================================//

void Fighter::apply_hit_generic(const HitBlob& hit, const HurtBlob& hurt)
{
    SQASSERT(&hit.action->fighter != this, "invalid hitblob");
    SQASSERT(hurt.fighter == this, "invalid hurtblob");

    const Attributes& attrs = attributes;
    Variables& vars = variables;
    Variables& otherVars = hit.action->fighter.variables;

    if (vars.intangible == true) return;

    // if we were in the middle of an action, cancel it
    cancel_action();

    //--------------------------------------------------------//

    // todo: investigate how stacking works when we were hit previously
    //       currently we just replace any existing freeze/stun/knockback

    const bool shielding = activeState->name.starts_with("Shield");

    // https://www.ssbwiki.com/Hitlag#Formula
    const uint freezeTime = [&]() {
        const float shield = shielding ? 0.75f : 1.f;
        const float time = (hit.damage * 0.5f + 4.f) * hit.freezeMult * shield;
        return maths::min(uint(time), 32u);
    }();

    // https://www.ssbwiki.com/Angle#Angle_flipper
    const float facing = [&]() {
        if (hit.facingMode == BlobFacingMode::Forward) return +float(otherVars.facing);
        if (hit.facingMode == BlobFacingMode::Reverse) return -float(otherVars.facing);
        return otherVars.position.x < vars.position.x ? +1.f : -1.f; // Relative
    }();

    //--------------------------------------------------------//

    vars.freezeTime = freezeTime;
    otherVars.freezeTime = freezeTime;

    // always start jittering with an even index
    mJitterCounter = mJitterCounter & 254u;

    // todo: add self to a list of somethings
    hit.action->set_hit_something();

    hit.action->wren_play_sound(hit.sound);

    //--------------------------------------------------------//

    if (shielding == true)
    {
        if ((vars.shield -= hit.damage) > 0.f)
        {
            vars.velocity.x += facing * (SHIELD_PUSH_HURT_BASE + hit.damage * SHIELD_PUSH_HURT_FACTOR);
            otherVars.velocity.x -= facing * (SHIELD_PUSH_HIT_BASE + hit.damage * SHIELD_PUSH_HIT_FACTOR);

            vars.stunTime = uint8_t(SHIELD_STUN_BASE + SHIELD_STUN_FACTOR * hit.damage);

            change_state(mStates.at("ShieldStun"));
        }
        else apply_shield_break(); // todo

        return;
    }

    //--------------------------------------------------------//

    // https://www.ssbwiki.com/Knockback#Melee_onward
    // knockback and weight values are the same arbitary units as in smash bros
    // hitstun should always end before launch speed completely decays
    // non-launching attacks can not cause tumbling

    vars.damage += hit.damage;

    // https://www.ssbwiki.com/Knockback#Melee_onward

    // todo: for ignore weight, set gravity and fall speed as in https://www.ssbwiki.com/Knockback#Ultimate

    const float knockback = [&]() {
        const float b = hit.ignoreDamage ? 0.f : hit.knockBase;
        const float d = hit.ignoreDamage ? hit.knockBase : hit.damage;
        const float p = hit.ignoreDamage ? 10.f : vars.damage;
        const float w = hit.ignoreWeight ? 100.f : attrs.weight;
        const float s = hit.knockScale;
        return (((p / 10.f + p * d / 20.f) * 200.f / (w + 100.f) * 1.4f + 18.f) * s * 0.01f + b);
    }();

    const float angle = [&]() {
        // https://www.ssbwiki.com/Sakurai_angle
        if (hit.angleMode == BlobAngleMode::Sakurai)
        {
            if (vars.onGround == false) return maths::radians(45.f / 360.f);
            if (knockback < 60.f) return 0.f;
            const float blend = std::clamp((knockback - 60.f) / 30.f, 0.f, 1.f);
            return maths::radians(maths::mix(10.f, 40.f, blend) / 360.f);
        }
        // https://www.ssbwiki.com/Autolink_angle
        if (hit.angleMode == BlobAngleMode::AutoLink)
        {
            // todo
            return maths::radians(hit.knockAngle / 360.f);
        }
        return maths::radians(hit.knockAngle / 360.f);
    }();

    const float launchSpeed = knockback * 0.003f;
    const uint hitStunTime = uint(knockback * 0.4f);

    const Vec2F knockDir = { std::cos(angle) * facing, std::sin(angle) };

    sq::log_debug_multiline("fighter {} hit by fighter {}:"
                            "\nknockback:   {}" "\nfreezeTime:  {}" "\nhitStun:     {}"
                            "\nknockDir:    {}" "\nlaunchSpeed: {}" "\ndecayFrames: {}"
                            "\nhitKey:      {}" "\nhurtKey:     {}" "\nhurtRegion:  {}",
                            index, hit.action->fighter.index,
                            knockback, freezeTime, hitStunTime,
                            knockDir, launchSpeed, uint(launchSpeed / KNOCKBACK_DECAY),
                            hit.get_key(), hurt.get_key(), hurt.region);

    SQASSERT(hitStunTime < uint(launchSpeed / KNOCKBACK_DECAY), "something is broken");

    vars.stunTime = hitStunTime;

    vars.velocity = maths::normalize(knockDir) * launchSpeed;

    vars.launchSpeed = launchSpeed;

    // rotate towards the camera over the duration of hitstun
    if (vars.facing == int8_t(facing))
        wren_reverse_facing_slow(vars.facing == 1, hitStunTime);

    const bool heavy = vars.stunTime >= MIN_HITSTUN_HEAVY;

    SmallString state, animNow, animAfter;

    if (vars.onGround == false || angle != 0.f)
    {
        if (vars.stunTime >= MIN_HITSTUN_TUMBLE)
        {
            state = "TumbleStun";
            if (hurt.region == BlobRegion::Middle) animNow = "HurtMiddleTumble";
            else if (hurt.region == BlobRegion::Lower) animNow = "HurtLowerTumble";
            else if (hurt.region == BlobRegion::Upper) animNow = "HurtUpperTumble";
            else SQEE_UNREACHABLE();
            animAfter = "TumbleLoop";
        }
        else
        {
            state = "FallStun";
            animNow = heavy ? "HurtAirHeavy" : "HurtAirLight";
            animAfter = "FallLoop";
        }
    }
    else
    {
        state = "NeutralStun";
        if (hurt.region == BlobRegion::Middle) animNow = heavy ? "HurtMiddleHeavy" : "HurtMiddleLight";
        else if (hurt.region == BlobRegion::Lower) animNow = heavy ? "HurtLowerHeavy" : "HurtLowerLight";
        else if (hurt.region == BlobRegion::Upper) animNow = heavy ? "HurtUpperHeavy" : "HurtUpperLight";
        else SQEE_UNREACHABLE();
        animAfter = "NeutralLoop";
    }

    change_state(mStates.at(state));
    play_animation(mAnimations.at(animNow), 0u, true);
    set_next_animation(mAnimations.at(animAfter), 0u);

    update_animation();

    mModelMatrix = maths::transform(current.translation, current.rotation);
    //mArmature.compute_ubo_data(current.pose, mBoneMatrices.data(), uint(mBoneMatrices.size()));
}

//============================================================================//

void Fighter::apply_shield_break()
{
    //mHitStunTime = 240u; // 5 seconds
    //mFrozenState =
}

//============================================================================//

void Fighter::pass_boundary()
{
    // todo: this should be its own action
    reset_everything();
    activeState = &mStates.at("Neutral");
    activeState->call_do_enter();
    play_animation(mAnimations.at("NeutralLoop"), 0u, true);
}

//============================================================================//

Mat4F Fighter::get_bone_matrix(int8_t bone) const
{
    SQASSERT(bone < int8_t(mArmature.get_bone_count()), "invalid bone");
    if (bone < 0) return mModelMatrix;

    // todo: in smash, it seems that scale and translation are ignored for the leaf bone
    // rukai data seems to ignore them for ALL bones in the chain, but that seems really wrong to me
    // see https://github.com/rukai/brawllib_rs/blob/main/src/high_level_fighter.rs#L537
    return mModelMatrix * maths::transpose(Mat4F(mBoneMatrices[bone]));
}

//============================================================================//

void Fighter::start_action(FighterAction& action)
{
    if (editorErrorCause != &action)
    {
        activeAction = &action;
        activeAction->call_do_start();
    }
    else activeAction = nullptr;
}

void Fighter::cancel_action()
{
    if (activeAction == nullptr) return;

    if (editorErrorCause != activeAction)
        activeAction->call_do_cancel();

    activeAction = nullptr;
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

void Fighter::play_animation(const Animation& animation, uint fade, bool fromStart)
{
    mAnimation = &animation;
    mNextAnimation = nullptr;

    if (bool(mRotateMode & RotateMode::Animation))
    {
        // end rotation if play_animation gets called twice
        if (bool(mRotateMode & RotateMode::Playing)) mRotateMode = RotateMode::Auto;
        else mRotateMode = mRotateMode | RotateMode::Playing;
    }

    mFadeFrames = fade;
    mNextFadeFrames = 0u;
    mFadeProgress = 0u;

    // don't set fade rotation if doing a slow or animated rotation
    if (mRotateMode == RotateMode::Auto) mFadeStartRotation = current.rotation;
    mFadeStartPose = current.pose;

    if (fromStart == true)
    {
        mAnimTimeDiscrete = 0u;
        mAnimTimeContinuous = 0.f;
        mRootMotionPreviousOffset = Vec3F();
    }
}

void Fighter::set_next_animation(const Animation& animation, uint fade)
{
    if (mAnimation != nullptr)
    {
        mNextAnimation = &animation;
        mNextFadeFrames = fade;
    }
    else play_animation(animation, fade, true);
}

//============================================================================//

void Fighter::reset_everything()
{
    if (activeAction) { activeAction->call_do_cancel(); activeAction = nullptr; }
    if (activeState) { activeState->call_do_exit(); activeState = nullptr; }
    variables = Variables();
    previous.translation = current.translation = Vec3F();
    previous.rotation = current.rotation = QuatF();
    previous.pose = current.pose = mArmature.get_rest_pose();
    mRootMotionPreviousOffset = Vec3F();
    mRootMotionTranslate = Vec2F();
    mRotateMode = RotateMode::Auto;
    editorErrorCause = nullptr;
    editorErrorMessage = "";
    editorStartAction = nullptr;
}
