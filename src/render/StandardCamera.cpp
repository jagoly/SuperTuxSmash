#include "render/StandardCamera.hpp"

#include "main/Options.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/Stage.hpp"

#include "render/Renderer.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

void StandardCamera::update_from_world(const FightWorld& world)
{
    const float border = 3.f * renderer.options.camera_zoom_out;

    mPreviousView = mCurrentView;
    mPreviousBounds = mCurrentBounds;

    mCurrentView = { Vec2F(+INFINITY), Vec2F(-INFINITY) };

    for (const Fighter* fighter : world.get_fighters())
    {
        if (fighter == nullptr) continue;

        const Vec2F centre = fighter->status.position + fighter->get_diamond().cross();

        mCurrentView.min = maths::min(mCurrentView.min, centre);
        mCurrentView.max = maths::max(mCurrentView.max, centre);
    }

    mCurrentView.min -= border;
    mCurrentView.max += border;

    //mCurrentBounds.min = world.get_stage().get_inner_boundary().min;
    //mCurrentBounds.max = world.get_stage().get_inner_boundary().max;

    mCurrentBounds.min = world.get_stage().get_outer_boundary().min;
    mCurrentBounds.max = world.get_stage().get_outer_boundary().max;

    mCurrentView.min = maths::max(mCurrentView.min, mCurrentBounds.min);
    mCurrentView.min = maths::min(mCurrentView.min, mCurrentBounds.max - border - border);

    mCurrentView.max = maths::min(mCurrentView.max, mCurrentBounds.max);
    mCurrentView.max = maths::max(mCurrentView.max, mCurrentBounds.min + border + border);

    MinMax averageView = mCurrentView;

    for (uint i = 0u; i < 15u; ++i)
    {
        averageView.min += mViewHistory[i].min;
        averageView.max += mViewHistory[i].max;
        mViewHistory[i] = mViewHistory[i + 1];
    }

    mViewHistory.back() = mCurrentView;

    if (renderer.options.camera_smooth == true)
    {
        mCurrentView.min = averageView.min / 16.f;
        mCurrentView.max = averageView.max / 16.f;
    }
}

//============================================================================//

void StandardCamera::intergrate(float blend)
{
    const float aspect = float(renderer.window.get_size().x) / float(renderer.window.get_size().y);

    mBlock.projMat = maths::perspective_LH(maths::radians(0.15f), aspect, 0.5f, 100.f);

    const Vec2F minView = maths::mix(mPreviousView.min, mCurrentView.min, blend);
    const Vec2F maxView = maths::mix(mPreviousView.max, mCurrentView.max, blend);

    const float cameraDistanceX = (maxView.x - minView.x) * 0.5f;
    const float cameraDistanceY = (maxView.y - minView.y) * 0.5f;

    const float cameraDistance = maths::max(cameraDistanceX / aspect, cameraDistanceY) / std::tan(0.5f);
    Vec3F cameraTarget = Vec3F((minView + maxView) * 0.5f, 0.f);

    const Vec2F minBounds = maths::mix(mPreviousBounds.min, mCurrentBounds.min, blend);
    const Vec2F maxBounds = maths::mix(mPreviousBounds.max, mCurrentBounds.max, blend);

    const auto clamp_to_bounds = [](float value, float min, float max, float distance)
    {
        min += distance; max -= distance;
        if (min > max) return (max + min) * 0.5f;
        return maths::clamp(value, min, max);
    };

    cameraTarget.x = clamp_to_bounds(cameraTarget.x, minBounds.x, maxBounds.x, cameraDistanceY * aspect);
    cameraTarget.y = clamp_to_bounds(cameraTarget.y, minBounds.y, maxBounds.y, cameraDistanceX / aspect);

    Vec3F position = cameraTarget - Vec3F(0.f, 0.f, cameraDistance);

    const float factorX = cameraTarget.x / (cameraTarget.x > 0.f ? maxBounds.x : -minBounds.x);
    const float factorY = cameraTarget.y / (cameraTarget.y > 0.f ? maxBounds.y : -minBounds.y);
    position.x += factorX * cameraDistance * 0.125f;
    position.y += factorY * cameraDistance * 0.125f;

    mBlock.viewMat = maths::look_at_LH(position, cameraTarget, Vec3F(0.f, 1.f, 0.f));

    mBlock.invViewMat = maths::inverse(mBlock.viewMat);
    mBlock.invProjMat = maths::inverse(mBlock.projMat);

    mBlock.projViewMat = mBlock.projMat * mBlock.viewMat;
}
