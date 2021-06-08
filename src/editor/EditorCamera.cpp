#include "editor/EditorCamera.hpp"

#include "main/Options.hpp"
#include "render/Renderer.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/maths/Functions.hpp>

using namespace sts;

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

//============================================================================//

void EditorCamera::intergrate(float /*blend*/)
{
    const float aspect = float(renderer.window.get_size().x) / float(renderer.window.get_size().y);

    mBlock.projMat = maths::perspective_LH(1.f, aspect, 0.5f, 100.f);

    const Mat3F rotation = maths::rotation({1.f, 0.f, 0.f}, mPitch) * maths::rotation({0.f, 1.f, 0.f}, mYaw);

    mBlock.position = rotation * Vec3F(0.f, 0.f, -mZoom);
    mBlock.position.x -= mCentre.x;
    mBlock.position.y -= mCentre.y;

    mBlock.direction = maths::normalize(rotation * Vec3F(0.f, 0.f, 1.f));

    mBlock.viewMat = maths::translate(maths::transform(Vec3F(0.f, 0.f, mZoom), rotation), Vec3F(-mCentre, 0.f));

    mBlock.invViewMat = maths::inverse(mBlock.viewMat);
    mBlock.invProjMat = maths::inverse(mBlock.projMat);

    mBlock.projViewMat = mBlock.projMat * mBlock.viewMat;
}
