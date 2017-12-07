#pragma once

#include "render/Renderer.hpp"

//============================================================================//

namespace sts {

class Camera final : sq::NonCopyable
{
public: //====================================================//

    Camera(Renderer& renderer);

    //--------------------------------------------------------//

    void update_from_scene_data(const SceneData& sceneData);

    void intergrate(float blend);

    //--------------------------------------------------------//

    const sq::UniformBuffer& get_ubo() const { return mCameraUbo; }

    const Mat4F& get_view_matrix() const { return mProjMatrix; }
    const Mat4F& get_proj_matrix() const { return mProjMatrix; }

    const Mat4F& get_inv_view_matrix() const { return mInvViewMatrix; }
    const Mat4F& get_inv_proj_matrix() const { return mInvProjMatrix; }

    const Mat4F& get_combo_matrix() const { return mComboMatrix; }

private: //===================================================//

    sq::UniformBuffer mCameraUbo;

    Mat4F mViewMatrix, mProjMatrix;
    Mat4F mInvViewMatrix, mInvProjMatrix;
    Mat4F mComboMatrix;

    //--------------------------------------------------------//

    struct MinMax { Vec2F min, max; };

    std::array<MinMax, 16u> mViewHistory;

    MinMax mPreviousView, mCurrentView;
    MinMax mPreviousBounds, mCurrentBounds;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
