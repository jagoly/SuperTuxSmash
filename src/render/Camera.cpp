#include "render/Camera.hpp"

using namespace sts;

//============================================================================//

Camera::Camera(const Renderer& renderer) : renderer(renderer) {}

Camera::~Camera() = default;
