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

    const CameraBlock& get_block() const { return mBlock; }

protected: //=================================================//

    const Renderer& renderer;

    CameraBlock mBlock;
};

//============================================================================//

} // namespace sts
