#include "render/Camera.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Misc.hpp>
#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>
#include <sqee/maths/Functions.hpp>

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Camera::Camera(const Renderer& renderer) : renderer(renderer)
{
    //-- Create Uniform Buffers ------------------------------//

    mUbo.create_and_allocate(sizeof(CameraBlock));
}

//============================================================================//

void StandardCamera::update_from_scene_data(const SceneData& sceneData)
{
    mPreviousView = mCurrentView;
    mPreviousBounds = mCurrentBounds;

    MinMax averageView = { sceneData.view.min, sceneData.view.max };

    //for (uint i = 0u; i < 1u; ++i)
    for (uint i = 0u; i < 15u; ++i)
    {
        averageView.min += mViewHistory[i].min;
        averageView.max += mViewHistory[i].max;
        mViewHistory[i] = mViewHistory[i + 1];
    }

    //mViewHistory[1] = { sceneData.view.min, sceneData.view.max };
    mViewHistory.back() = { sceneData.view.min, sceneData.view.max };

    //mCurrentView.min = averageView.min / 2.f;
    //mCurrentView.max = averageView.max / 2.f;
    mCurrentView.min = averageView.min / 16.f;
    mCurrentView.max = averageView.max / 16.f;

    mCurrentBounds = { sceneData.outer.min, sceneData.outer.max };
}

//----------------------------------------------------------------------------//

void StandardCamera::intergrate(float blend)
{
    const float aspect = float(renderer.options.Window_Size.x) / float(renderer.options.Window_Size.y);

    mBlock.direction = maths::normalize(Vec3F(0.f, -0.f, +4.f));

    mBlock.projMat = maths::perspective_LH(1.f, aspect, 0.2f, 200.f);

    const Vec2F minView = maths::mix(mPreviousView.min, mCurrentView.min, blend);
    const Vec2F maxView = maths::mix(mPreviousView.max, mCurrentView.max, blend);

    const Vec2F minBounds = maths::mix(mPreviousBounds.min, mCurrentBounds.min, blend);
    const Vec2F maxBounds = maths::mix(mPreviousBounds.max, mCurrentBounds.max, blend);

    const float cameraDistance = [&]() {
        float distanceX = (maxView.x - minView.x + 4.f) * 0.5f / std::tan(0.5f) / aspect;
        float distanceY = (maxView.y - minView.y + 4.f) * 0.5f / std::tan(0.5f);
        return maths::max(distanceX, distanceY); }();

    Vec3F cameraTarget = Vec3F((minView + maxView) * 0.5f, 0.f);
    mBlock.position = cameraTarget - mBlock.direction * cameraDistance;

    mBlock.viewMat = maths::look_at_LH(mBlock.position, cameraTarget, Vec3F(0.f, 1.f, 0.f));

    const float screenLeft = [&]() {
        Vec4F worldPos = Vec4F(minBounds.x, cameraTarget.y, 0.f, 1.f);
        Vec4F screenPos = mBlock.projMat * (mBlock.viewMat * worldPos);
        return (screenPos.x / screenPos.w + 1.f) * screenPos.w; }();

    const float screenRight = [&]() {
        Vec4F worldPos = Vec4F(maxBounds.x, cameraTarget.y, 0.f, 1.f);
        Vec4F screenPos = mBlock.projMat * (mBlock.viewMat * worldPos);
        return (screenPos.x / screenPos.w - 1.f) * screenPos.w; }();

    if (screenLeft > 0.f && screenRight < 0.f) {}
    else if (screenLeft > 0.f) { cameraTarget.x += screenLeft; }
    else if (screenRight < 0.f) { cameraTarget.x += screenRight; }

    mBlock.position = cameraTarget - mBlock.direction * cameraDistance;

    mBlock.viewMat = maths::look_at_LH(mBlock.position, cameraTarget, Vec3F(0.f, 1.f, 0.f));

    //--------------------------------------------------------//

    mBlock.invViewMat = maths::inverse(mBlock.viewMat);
    mBlock.invProjMat = maths::inverse(mBlock.projMat);

    mUbo.update(0u, mBlock);

    //--------------------------------------------------------//

    mComboMatrix = mBlock.projMat * mBlock.viewMat;
}

//============================================================================//

void EditorCamera::update_from_scroll(float delta)
{
    mZoom -= delta / 4.f;
    mZoom = maths::clamp(mZoom, 1.f, 8.f);
}

void EditorCamera::update_from_mouse(bool left, bool right, Vec2F position)
{
    if (left && !right)
    {
        mYaw -= (position.x - mPrevMousePosition.x) / 600.f;
        mPitch += (position.y - mPrevMousePosition.y) / 800.f;
        mYaw = maths::clamp(mYaw, -0.2f, 0.2f);
        mPitch = maths::clamp(mPitch, -0.05f, 0.2f);
    }

    if (right && !left)
    {
        mCentre.x -= (position.x - mPrevMousePosition.x) / 250.f;
        mCentre.y -= (position.y - mPrevMousePosition.y) / 250.f;
        mCentre.x = maths::clamp(mCentre.x, -3.f, 3.f);
        mCentre.y = maths::clamp(mCentre.y, -1.f, 3.f);
    }

    mPrevMousePosition = position;
}

//----------------------------------------------------------------------------//

void EditorCamera::intergrate(float blend [[maybe_unused]])
{
    const float aspect = float(renderer.options.Window_Size.x) / float(renderer.options.Window_Size.y);

    mBlock.projMat = maths::perspective_LH(1.f, aspect, 0.2f, 200.f);

    const Mat3F rotation = maths::rotation({1.f, 0.f, 0.f}, mPitch) * maths::rotation({0.f, 1.f, 0.f}, mYaw);

    mBlock.position = rotation * Vec3F(0.f, 0.f, -mZoom);
    mBlock.position.x -= mCentre.x;
    mBlock.position.y -= mCentre.y;

    mBlock.direction = maths::normalize(rotation * Vec3F(0.f, 0.f, 1.f));

    mBlock.viewMat = maths::translate(maths::transform(Vec3F(0.f, 0.f, mZoom), rotation), Vec3F(-mCentre, 0.f));

    //--------------------------------------------------------//

    mBlock.invViewMat = maths::inverse(mBlock.viewMat);
    mBlock.invProjMat = maths::inverse(mBlock.projMat);

    mUbo.update(0u, mBlock);

    //--------------------------------------------------------//

    mComboMatrix = mBlock.projMat * mBlock.viewMat;
}
