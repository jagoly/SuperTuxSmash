#pragma once

#include "setup.hpp"

#include "render/UniformBlocks.hpp" // IWYU pragma: export

namespace sts {

//============================================================================//

class Camera : sq::NonCopyable
{
public: //====================================================//

    Camera(const Renderer& renderer);

    virtual ~Camera();

    virtual void intergrate(float blend) = 0;

    //--------------------------------------------------------//

    /// Compute a tight fitting projView matrix for shadow mapping.
    Mat4F compute_light_matrix(const Mat4F& lightViewMat, Vec3F stageMin, Vec3F stageMax) const;

    //--------------------------------------------------------//

    /// Get the camera's position from the inverse view matrix.
    Vec3F extract_position() const { return Vec3F(mBlock.invViewMat[3]); }

    /// Get the camera's direction from the view matrix.
    Vec3F extract_direction() const { return Vec3F(mBlock.viewMat[0][2], mBlock.viewMat[1][2], mBlock.viewMat[2][2]); }

    /// Access any of the camera's matrices.
    const CameraBlock& get_block() const { return mBlock; }

protected: //=================================================//

    const Renderer& renderer;

    CameraBlock mBlock;
};

//============================================================================//

} // namespace sts
