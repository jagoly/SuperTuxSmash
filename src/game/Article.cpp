#include "game/Article.hpp"

#include "game/Physics.hpp"
#include "game/Stage.hpp"
#include "game/World.hpp"

#include "render/Renderer.hpp"

#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

Article::Article(const ArticleDef& def, Fighter* fighter)
    : Entity(def), def(def), fighter(fighter)
{
    // scriptClass.new is done from wren in wren to prevent reentrance
}

Article::~Article()
{
    if (mScriptHandle) wrenReleaseHandle(world.vm, mScriptHandle);
    if (mFiberHandle) wrenReleaseHandle(world.vm, mFiberHandle);
}

//============================================================================//

void Article::call_do_updates()
{
    const auto error = world.vm.safe_call_void(world.handles.article_do_updates, this);
    if (error.empty() == false)
        set_error_message("call_do_updates", error);
}

//============================================================================//

void Article::call_do_destroy()
{
    const auto error = world.vm.safe_call_void(world.handles.article_do_destroy, this);
    if (error.empty() == false)
        set_error_message("call_do_destroy", error);
}

//============================================================================//

void Article::set_error_message(StringView method, StringView errors)
{
    String message = fmt::format (
        "'{}'\n{}C++ | {}() | frame = {}\n", def.directory, errors, method, mCurrentFrame
    );

    if (world.editor == nullptr)
        sq::log_error_multiline(message);

    else if (world.editor->ctxKey != def.directory)
        sq::log_warning_multiline(message);

    // only show the first error, which usually causes the other errors
    else if (world.editor->errorMessage.empty())
        world.editor->errorMessage = std::move(message);
}

//============================================================================//

void Article::tick()
{
    // won't have a script if its constructor aborted
    if (mScriptHandle == nullptr)
    {
        SQASSERT(world.editor != nullptr, "");
        mMarkedForDestroy = true;
        return;
    }

    //--------------------------------------------------------//

    Variables& vars = variables;

    // todo: consider giving article scripts either an update_frozen
    //       method, or a callback for when they hit something
    if (vars.hitSomething == true && vars.fragile == true)
        mMarkedForDestroy = true;

    if (mMarkedForDestroy == true)
    {
        for (int32_t id : mTransientEffects)
            world.wren_cancel_effect(id);

        for (int32_t id : mTransientSounds)
            world.wren_cancel_sound(id);

        call_do_destroy();

        return;
    }

    //--------------------------------------------------------//

    previous = current;

    if (vars.freezeTime == 0u)
    {
        // todo: should we expose attempt_move_sphere to wren and call it from the script?
        const MoveAttemptSphere moveAttempt = world.get_stage().attempt_move_sphere(0.3f, 0.75f, vars.position, vars.velocity, false);

        vars.position = moveAttempt.newPosition;
        vars.velocity = moveAttempt.newVelocity;
        vars.bounced = moveAttempt.bounced;

        call_do_updates();

        if (mMarkedForDestroy == true)
        {
            for (int32_t id : mTransientEffects)
                world.wren_cancel_effect(id);

            for (int32_t id : mTransientSounds)
                world.wren_cancel_sound(id);

            call_do_destroy();

            return;
        }

        std::swap(mAnimPlayer.previousSample, mAnimPlayer.currentSample);

        update_animation();
    }
    else
    {
        mAnimPlayer.previousSample = mAnimPlayer.currentSample;
        debugPreviousPoseInfo = debugCurrentPoseInfo;

        --vars.freezeTime;
    }

    // make sure previous has valid values to interpolate from
    if (mJustCreated == true)
    {
        mJustCreated = false;
        previous = current;
        mAnimPlayer.previousSample = mAnimPlayer.currentSample;
        // todo: add more options for this
        reinterpret_cast<sq::Armature::Bone*>(mAnimPlayer.previousSample.data())->scale = Vec3F();
    }

    // always compute updated model matrix
    mModelMatrix = maths::transform(current.translation, current.rotation);
}

//============================================================================//

void Article::integrate(float blend)
{
    integrate_base(blend);

    const auto check_condition = [&](const TinyString& condition)
    {
        if (condition.empty()) return true;
        return true; // invalid
    };

    for (const sq::DrawItem& item : def.drawItems)
        if (check_condition(item.condition) == true)
            world.renderer.add_draw_call(item, mAnimPlayer);
}
