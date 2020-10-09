#include "render/Camera.hpp"

using namespace sts;

//============================================================================//

Camera::Camera(const Renderer& renderer) : renderer(renderer)
{
    //-- Create Uniform Buffers ------------------------------//

    mUbo.allocate_dynamic(sizeof(CameraBlock));
}
