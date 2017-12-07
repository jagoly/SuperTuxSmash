#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Misc.hpp>

#include <sqee/maths/Functions.hpp>

#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>

#include "render/Camera.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Camera::Camera(Renderer& renderer) : renderer(renderer)
{
    //-- Create Uniform Buffers ------------------------------//

    mCameraUbo.create_and_allocate(288u);
}

//============================================================================//

void Camera::update_from_scene_data(const SceneData& sceneData)
{
    mPreviousView = mCurrentView;
    mPreviousBounds = mCurrentBounds;

    MinMax averageView = { sceneData.view.min, sceneData.view.max };

    for (uint i = 0u; i < 15u; ++i)
    {
        averageView.min += mViewHistory[i].min;
        averageView.max += mViewHistory[i].max;
        mViewHistory[i] = mViewHistory[i + 1];
    }

    mViewHistory.back() = { sceneData.view.min, sceneData.view.max };

    mCurrentView.min = averageView.min / 16.f;
    mCurrentView.max = averageView.max / 16.f;

    mCurrentBounds = { sceneData.outer.min, sceneData.outer.max };
}

//============================================================================//

void Camera::intergrate(float blend)
{
    const float aspect = float(renderer.options.Window_Size.x) / float(renderer.options.Window_Size.y);

    const Vec3F cameraDirection = maths::normalize(Vec3F(0.f, -0.f, +4.f));

    mProjMatrix = maths::perspective_LH(1.f, aspect, 0.2f, 200.f);

    const Vec2F minView = maths::mix(mPreviousView.min, mCurrentView.min, blend);
    const Vec2F maxView = maths::mix(mPreviousView.max, mCurrentView.max, blend);

    const Vec2F minBounds = maths::mix(mPreviousBounds.min, mCurrentBounds.min, blend);
    const Vec2F maxBounds = maths::mix(mPreviousBounds.max, mCurrentBounds.max, blend);

    const float cameraDistance = [&]() {
        float distanceX = (maxView.x - minView.x + 4.f) * 0.5f / std::tan(0.5f) / aspect;
        float distanceY = (maxView.y - minView.y + 4.f) * 0.5f / std::tan(0.5f);
        return maths::max(distanceX, distanceY); }();

    Vec3F cameraTarget = Vec3F((minView + maxView) * 0.5f, 0.f);
    Vec3F cameraPosition = cameraTarget - cameraDirection * cameraDistance;

    mViewMatrix = maths::look_at_LH(cameraPosition, cameraTarget, Vec3F(0.f, 1.f, 0.f));

    const float screenLeft = [&]() {
        Vec4F worldPos = Vec4F(minBounds.x, cameraTarget.y, 0.f, 1.f);
        Vec4F screenPos = mProjMatrix * (mViewMatrix * worldPos);
        return (screenPos.x / screenPos.w + 1.f) * screenPos.w; }();

    const float screenRight = [&]() {
        Vec4F worldPos = Vec4F(maxBounds.x, cameraTarget.y, 0.f, 1.f);
        Vec4F screenPos = mProjMatrix * (mViewMatrix * worldPos);
        return (screenPos.x / screenPos.w - 1.f) * screenPos.w; }();

    if (screenLeft > 0.f && screenRight < 0.f) {}
    else if (screenLeft > 0.f) { cameraTarget.x += screenLeft; }
    else if (screenRight < 0.f) { cameraTarget.x += screenRight; }

    cameraPosition = cameraTarget - cameraDirection * cameraDistance;

    mViewMatrix = maths::look_at_LH(cameraPosition, cameraTarget, Vec3F(0.f, 1.f, 0.f));

    //--------------------------------------------------------//

    mInvViewMatrix = maths::inverse(mViewMatrix);
    mInvProjMatrix = maths::inverse(mProjMatrix);

    mComboMatrix = mProjMatrix * mViewMatrix;

    mCameraUbo.update_complete ( mViewMatrix, mProjMatrix, mInvViewMatrix, mInvProjMatrix,
                                 cameraPosition, 0, cameraDirection, 0 );
}
