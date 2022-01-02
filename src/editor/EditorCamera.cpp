#include "editor/EditorCamera.hpp"

#include "main/Options.hpp"
#include "render/Renderer.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/maths/Functions.hpp>

#include <dearimgui/imgui.h>

using namespace sts;

//============================================================================//

void EditorCamera::update_from_world(const FightWorld& /*world*/) {}

//============================================================================//

void EditorCamera::update_from_controller(const Controller& /*controller*/) {}

//============================================================================//

void EditorCamera::integrate(float /*blend*/)
{
    const ImGuiIO& io = ImGui::GetIO();

    if (io.WantCaptureMouse == false)
    {
        if (io.MouseDown[ImGuiMouseButton_Left] && !io.MouseDown[ImGuiMouseButton_Right])
        {
            mYaw = std::clamp(mYaw - io.MouseDelta.x / 600.f, -0.25f, +0.25f);
            mPitch = std::clamp(mPitch - io.MouseDelta.y / 600.f, -0.25f, +0.25f);
        }

        if (io.MouseDown[ImGuiMouseButton_Right] && !io.MouseDown[ImGuiMouseButton_Left])
        {
            // todo: context should set speed and bounds
            // actions should have a small range around the fighter, and should move when the fighter does
            // stages should allow moving around the entire stage
            mCentre.x = std::clamp(mCentre.x - io.MouseDelta.x / 250.f, -10.f, +10.f);
            mCentre.y = std::clamp(mCentre.y + io.MouseDelta.y / 250.f, -2.f, +8.f);
        }

        mZoom = std::clamp(mZoom - io.MouseWheel / 4.f, 1.f, 16.f);
    }

    const float aspect = float(renderer.window.get_size().x) / float(renderer.window.get_size().y);

    mBlock.projMat = maths::perspective_LH(maths::radians(0.15f), aspect, 0.5f, 100.f);

    const Mat3F rotation = maths::rotation({1.f, 0.f, 0.f}, mPitch) * maths::rotation({0.f, 1.f, 0.f}, mYaw);

    mBlock.viewMat = maths::translate(maths::transform(Vec3F(0.f, 0.f, mZoom), rotation), Vec3F(-mCentre, 0.f));

    mBlock.invViewMat = maths::inverse(mBlock.viewMat);
    mBlock.invProjMat = maths::inverse(mBlock.projMat);

    mBlock.projViewMat = mBlock.projMat * mBlock.viewMat;
}
