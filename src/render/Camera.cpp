#include "render/Camera.hpp"

using namespace sts;

//============================================================================//

Camera::Camera(const Renderer& renderer) : renderer(renderer)
{
    //-- Create Uniform Buffers ------------------------------//

    mUbo.create_and_allocate(sizeof(CameraBlock));
}
