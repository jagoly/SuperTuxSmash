#include "render/RenderStage.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"

#include "game/Stage.hpp"

#include <sqee/gl/Context.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/objects/Mesh.hpp>

using namespace sts;

//============================================================================//

RenderStage::RenderStage(Renderer& renderer, const Stage& stage)
    : RenderObject(renderer), stage(stage)
{
    mUbo.allocate_dynamic(sizeof(StaticBlock));

    const String path = sq::build_string("assets/stages/", sq::enum_to_string(stage.type), "/Render.json");
    base_load_from_json(path, {});
}

//============================================================================//

void RenderStage::integrate(float /*blend*/)
{
    StaticBlock block;

    block.matrix = renderer.get_camera().get_combo_matrix();
    block.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_view_matrix()));

    mUbo.update(0u, block);
}

//============================================================================//

void RenderStage::render_opaque() const
{
    renderer.context.bind_buffer(mUbo, sq::BufTarget::Uniform, 2u);
    base_render_opaque();
}

void RenderStage::render_transparent() const
{
}
