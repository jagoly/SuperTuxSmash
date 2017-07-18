#include "render/RenderEntity.hpp"

using namespace sts;

//============================================================================//

RenderEntity::RenderEntity(const Entity& entity, const Renderer& renderer)
    : entity(entity), renderer(renderer) {}
