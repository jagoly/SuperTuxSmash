#include "render/StandardCamera.hpp"

#include "main/Options.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"
#include "game/Stage.hpp"

#include "render/Renderer.hpp"

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
    const float aspect = float(renderer.options.window_size.x) / float(renderer.options.window_size.y);

    mBlock.direction = Vec3F(0.f, 0.f, +1.f);

    mBlock.projMat = maths::perspective_LH(1.f, aspect, 0.2f, 200.f);

    const Vec2F minView = maths::mix(mPreviousView.min, mCurrentView.min, blend);
    const Vec2F maxView = maths::mix(mPreviousView.max, mCurrentView.max, blend);

    const float cameraDistanceX = (maxView.x - minView.x) * 0.5f / std::tan(0.5f) / aspect;
    const float cameraDistanceY = (maxView.y - minView.y) * 0.5f / std::tan(0.5f);

    const float cameraDistance = maths::max(cameraDistanceX, cameraDistanceY);
    const Vec3F cameraTarget = Vec3F((minView + maxView) * 0.5f, 0.f);

    mBlock.position = cameraTarget - mBlock.direction * cameraDistance;

    mBlock.viewMat = maths::look_at_LH(mBlock.position, cameraTarget, Vec3F(0.f, 1.f, 0.f));

    mBlock.invViewMat = maths::inverse(mBlock.viewMat);
    mBlock.invProjMat = maths::inverse(mBlock.projMat);

    mUbo.update(0u, mBlock);

    mComboMatrix = mBlock.projMat * mBlock.viewMat;
}
