#pragma once

#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"

//============================================================================//

namespace sts {

class Camera : sq::NonCopyable
{
public: //====================================================//

    Camera(const Renderer& renderer);
    virtual ~Camera() = default;

    //--------------------------------------------------------//

    virtual void intergrate(float blend) = 0;

    //--------------------------------------------------------//

    const sq::UniformBuffer& get_ubo() const { return mUbo; }

    const Mat4F& get_view_matrix() const { return mBlock.viewMat; }
    const Mat4F& get_proj_matrix() const { return mBlock.projMat; }

    const Mat4F& get_inv_view_matrix() const { return mBlock.invViewMat; }
    const Mat4F& get_inv_proj_matrix() const { return mBlock.invProjMat; }

    const Mat4F& get_combo_matrix() const { return mComboMatrix; }

protected: //=================================================//

    CameraBlock mBlock;
    sq::UniformBuffer mUbo;

    Mat4F mComboMatrix;

    const Renderer& renderer;
};

//============================================================================//

class StandardCamera final : public Camera
{
public: //====================================================//

    using Camera::Camera;

    void update_from_scene_data(const SceneData& sceneData);

    void intergrate(float blend) override;

    float zoomOut = 1.f;

    bool smoothMoves = true;

private: //===================================================//

    struct MinMax { Vec2F min, max; };

    Array<MinMax, 16u> mViewHistory;

    MinMax mPreviousView, mCurrentView;
    MinMax mPreviousBounds, mCurrentBounds;
};

//============================================================================//

class EditorCamera final : public Camera
{
public: //====================================================//

    using Camera::Camera;

    void update_from_scroll(float delta);

    void update_from_mouse(bool left, bool right, Vec2F position);

    void intergrate(float blend) override;

private: //===================================================//

    Vec2F mPrevMousePosition;

    float mYaw = 0.f;
    float mPitch = 0.f;
    float mZoom = 4.f;
    Vec2F mCentre = { 0.f, 1.f };
};

//============================================================================//

} // namespace sts
