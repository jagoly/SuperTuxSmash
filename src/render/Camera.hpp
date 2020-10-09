#pragma once

#include "setup.hpp"

#include "render/UniformBlocks.hpp" // IWYU pragma: export

#include <sqee/gl/FixedBuffer.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

class Camera : sq::NonCopyable
{
public: //====================================================//

    Camera(const Renderer& renderer);
    virtual ~Camera() = default;

    //--------------------------------------------------------//

    virtual void intergrate(float blend) = 0;

    //--------------------------------------------------------//

    const sq::FixedBuffer& get_ubo() const { return mUbo; }

    const Mat4F& get_view_matrix() const { return mBlock.viewMat; }
    const Mat4F& get_proj_matrix() const { return mBlock.projMat; }

    const Mat4F& get_inv_view_matrix() const { return mBlock.invViewMat; }
    const Mat4F& get_inv_proj_matrix() const { return mBlock.invProjMat; }

    const Mat4F& get_combo_matrix() const { return mComboMatrix; }

protected: //=================================================//

    CameraBlock mBlock;
    sq::FixedBuffer mUbo;

    Mat4F mComboMatrix;

    const Renderer& renderer;
};

//============================================================================//

} // namespace sts
