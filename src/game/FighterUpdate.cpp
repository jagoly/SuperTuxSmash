#include "game/Fighter.hpp" // IWYU pragma: associated

#include "game/Controller.hpp"
#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/Stage.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>

// todo: merge this file back into Fighter.cpp

using namespace sts;

//============================================================================//

void Fighter::update_action()
{
    // editor wants to restart an action
    if (editorStartAction != nullptr)
    {
        cancel_action();
        start_action(*editorStartAction);
        editorStartAction = nullptr;
    }

    // update normally
    else if (activeAction != nullptr /*&& activeAction != editorErrorCause*/)
        activeAction->call_do_updates();
}

void Fighter::update_state()
{
    // todo: editor can't deal with state errors yet, so ignore them for now
    if (activeState != nullptr /*&& activeState != editorErrorCause*/)
        activeState->call_do_updates();
}

//============================================================================//

void Fighter::update_movement()
{
    // attach anim handles movement on its own
    if (mAnimPlayer.animation != nullptr && mAnimPlayer.animation->attach == true)
        return;

    const Attributes& attrs = attributes;
    Variables& vars = variables;
    const InputFrame& input = controller->history.frames.front();
    Stage& stage = world.get_stage();

    //-- apply knockback decay -------------------------------//

    if (vars.launchSpeed != 0.f)
    {
        const float actualSpeed = maths::length(vars.velocity);
        const float decay = maths::min(vars.launchSpeed, KNOCKBACK_DECAY);

        vars.launchSpeed -= decay;

        if (actualSpeed <= decay) vars.velocity = Vec2F();
        else vars.velocity *= (actualSpeed - decay) / actualSpeed;
    }

    //-- apply friction --------------------------------------//

    if (vars.applyFriction == true)
    {
        if (vars.onGround == true)
        {
            if (vars.velocity.x < -0.f) vars.velocity.x = maths::min(vars.velocity.x + attrs.traction, -0.f);
            if (vars.velocity.x > +0.f) vars.velocity.x = maths::max(vars.velocity.x - attrs.traction, +0.f);
        }
        else if (vars.launchSpeed == 0.f && (vars.moveMobility == 0.f || input.intX == 0))
        {
            if (vars.velocity.x < -0.f) vars.velocity.x = maths::min(vars.velocity.x + attrs.airFriction, -0.f);
            if (vars.velocity.x > +0.f) vars.velocity.x = maths::max(vars.velocity.x - attrs.airFriction, +0.f);
        }
    }

    //-- apply horizontal movement ---------------------------//

    if (vars.moveMobility != 0.f && (vars.onGround == false || input.relIntX > 0))
    {
        const float targetValue = input.floatX * vars.moveSpeed;

        if (input.intX < 0 && vars.velocity.x > targetValue)
            vars.velocity.x = maths::max(vars.velocity.x - vars.moveMobility, targetValue);

        if (input.intX > 0 && vars.velocity.x < input.floatX * vars.moveSpeed)
            vars.velocity.x = maths::min(vars.velocity.x + vars.moveMobility, targetValue);
    }

    //-- apply gravity ---------------------------------------//

    if (vars.applyGravity == true)
    {
        const float targetValue = vars.fastFall ? -attrs.fastFallSpeed : -attrs.fallSpeed;

        // falling faster than maximum speed
        if (vars.velocity.y < targetValue)
        {
            // if we were meteor smashed, we have to wait for launch speed to decay on its own
            // otherwise, apply half gravity upwards (no idea what smash does)
            if (vars.launchSpeed == 0.f)
                vars.velocity.y = maths::min(vars.velocity.y + attrs.gravity * 0.5f, targetValue);
        }

        // apply gravity normally
        else if (vars.fastFall == false)
            vars.velocity.y = maths::max(vars.velocity.y - attrs.gravity, targetValue);

        // fast fall applies instantly
        else vars.velocity.y = targetValue;
    }

    // when gravity is disabled, reduce velocity towards zero
    else if (vars.velocity.y < -0.f)
        vars.velocity.y = maths::min(vars.velocity.y + attrs.gravity, -0.f);
    else if (vars.velocity.y > +0.f)
        vars.velocity.y = maths::max(vars.velocity.y - attrs.gravity, +0.f);

    //-- ask the stage where we can move ---------------------//

    const Vec2F translation = vars.velocity + mRootMotionTranslate;
    const Vec2F targetPosition = vars.position + translation;
    mRootMotionTranslate = Vec2F();

    const TinyString& stateName = activeState->def.name;

    const bool edgeStop = vars.onGround == false ? false :
                          vars.edgeStop == EdgeStopMode::Never ? false : vars.edgeStop == EdgeStopMode::Always ? true :
                          !(input.intX <= -3 && translation.x < -0.f) && !(input.intX >= +3 && translation.x > +0.f);

    // todo: move this to a variable for states to set from wren
    const bool ignorePlatforms = input.intY <= -3 &&
                                 (stateName == "Fall" || stateName == "FallStun" || stateName == "Helpless");

    const MoveAttempt moveAttempt = stage.attempt_move(localDiamond, vars.position, targetPosition, edgeStop, ignorePlatforms);

    vars.position = moveAttempt.result;
    vars.onPlatform = moveAttempt.onPlatform;
    vars.edge = moveAttempt.edge;

    // todo: teching, bouncing (when launched)

    if (moveAttempt.collideFloor == true)
    {
        // prevent getting caught moving up over corners
        if ((vars.onGround |= vars.velocity.y <= 0.f) == true)
            vars.velocity.y = 0.f;
    }
    else vars.onGround = false;

    if (moveAttempt.collideCeiling == true)
        vars.velocity.y = 0.f;

    // todo: I have a feeling smash doesn't let you accelerate into a wall
    //if (moveAttempt.collideWall == true)
    //    vars.velocity.x = 0.f;
}

//============================================================================//

void Fighter::update_misc()
{
    Variables& vars = variables;
    const TinyString& stateName = activeState->def.name;

    //-- decay or regenerate our shield ----------------------//

    // todo: this stuff should be wren, but I still need a good way to do constants

    if (stateName == "Shield")
        vars.shield = maths::max(vars.shield - SHIELD_DECAY, 0.f);

    else if (stateName != "ShieldStun")
        vars.shield = maths::min(vars.shield + SHIELD_REGEN, SHIELD_MAX_HP);
}

//============================================================================//

void Fighter::update_jitter()
{
    Variables& vars = variables;

    if (vars.flinch == true && activeState->def.name != "ShieldStun")
    {
        constexpr auto jitterValues = std::array
        {
            Vec2F(-0.8f, -0.8f), Vec2F(-0.4f, +0.1f), Vec2F( 0.0f, +1.0f), Vec2F(+0.4f, +0.1f),
            Vec2F(+0.8f, -0.8f), Vec2F(-0.1f, -0.4f), Vec2F(-1.0f,  0.0f), Vec2F(-0.1f, +0.4f),
            Vec2F(+0.8f, +0.8f), Vec2F(+0.4f, -0.1f), Vec2F( 0.0f, -1.0f), Vec2F(-0.4f, -0.1f),
            Vec2F(-0.8f, +0.8f), Vec2F(+0.1f, +0.4f), Vec2F(+1.0f,  0.0f), Vec2F(+0.1f, -0.4f),
        };

        const uint8_t jitterIndex = mJitterCounter % 16u;
        const float jitterStrength = float(vars.freezeTime) * 0.125f / 32.f + 0.0625f;
        const Vec2F jitter = jitterValues[jitterIndex] * jitterStrength;

        current.translation = Vec3F(vars.position + jitter, 0.f);
        mJitterCounter = uint8_t(int8_t(mJitterCounter) - vars.facing);
    }
}

//============================================================================//

void Fighter::tick()
{
    Variables& vars = variables;

    // set relative x for the newly added input frame
    controller->history.frames.front().set_relative_x(vars.facing);

    // check if we have crossed the stage boundary
    if (world.get_stage().check_point_out_of_bounds(vars.position + localDiamond.cross()))
        pass_boundary();

    // finish slow or animated rotation
    if (bool(mRotateMode & RotateMode::Done))
    {
        mRotateSlowTime = mRotateSlowProgress = 0u;
        mRotateMode = RotateMode::Auto;
    }

    previous = current;

    // do updates
    if (vars.freezeTime == 0u)
    {
        // will be overwritten so don't need to copy
        std::swap(mAnimPlayer.previousSample, mAnimPlayer.currentSample);

        update_movement();
        update_misc();
        update_action();
        update_state();

        update_animation();
    }
    else
    {
        mAnimPlayer.previousSample = mAnimPlayer.currentSample;
        debugPreviousPoseInfo = debugCurrentPoseInfo;

        update_jitter();

        --vars.freezeTime;
    }

    // always compute updated model matrix
    mModelMatrix = maths::transform(current.translation, current.rotation);
}

//============================================================================//

void Fighter::integrate(float blend)
{
    integrate_base(blend);

    const auto check_condition = [&](const TinyString& condition)
    {
        if (condition.empty()) return true;
        if (condition == "flinch")  return variables.flinch == true;
        if (condition == "!flinch") return variables.flinch == false;
        SQASSERT(false, "invalid condition");
    };

    for (const sq::DrawItem& item : def.drawItems)
        if (check_condition(item.condition) == true)
            world.renderer.add_draw_call(item, mAnimPlayer);
}
