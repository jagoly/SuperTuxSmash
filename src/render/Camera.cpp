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

void StandardCamera::update_from_scene_data(const SceneData& scene)
{
    const float border = 3.f * zoomOut;

    mPreviousView = mCurrentView;
    mPreviousBounds = mCurrentBounds;

    mCurrentView.min = scene.view.min - border;
    mCurrentView.max = scene.view.max + border;

    mCurrentBounds.min = scene.outer.min;
    mCurrentBounds.max = scene.outer.max;

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

    if (smoothMoves == true)
    {
        mCurrentView.min = averageView.min / 16.f;
        mCurrentView.max = averageView.max / 16.f;
    }
}

//----------------------------------------------------------------------------//

void StandardCamera::intergrate(float blend)
{
    const float aspect = float(renderer.options.Window_Size.x) / float(renderer.options.Window_Size.y);

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

//============================================================================//

void EditorCamera::update_from_scroll(float delta)
{
    mZoom -= delta / 4.f;
    mZoom = maths::clamp(mZoom, 1.f, 16.f);
}

void EditorCamera::update_from_mouse(bool left, bool right, Vec2F position)
{
    if (left && !right)
    {
        mYaw -= (position.x - mPrevMousePosition.x) / 600.f;
        mPitch += (position.y - mPrevMousePosition.y) / 800.f;
        mYaw = maths::clamp(mYaw, -0.25f, 0.25f);
        mPitch = maths::clamp(mPitch, -0.25f, 0.25f);
    }

    if (right && !left)
    {
        mCentre.x -= (position.x - mPrevMousePosition.x) / 250.f;
        mCentre.y -= (position.y - mPrevMousePosition.y) / 250.f;
        mCentre.x = maths::clamp(mCentre.x, -4.f, 4.f);
        mCentre.y = maths::clamp(mCentre.y, -1.f, 6.f);
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
