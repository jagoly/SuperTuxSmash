#include "render/RenderFighter.hpp"

#include "game/Fighter.hpp"
#include "game/FightWorld.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

RenderFighter::RenderFighter(Renderer& renderer, const Fighter& fighter)
    : RenderObject(renderer), fighter(fighter)
{
    mUbo.allocate_dynamic(64u + 48u + 48u * fighter.get_armature().get_bone_count());

    const String path = sq::build_string("assets/fighters/", sq::enum_to_string(fighter.type), "/Render.json");

    base_load_from_json(path, { {"flinch", &mConditionFlinch} });
}

//============================================================================//

void RenderFighter::integrate(float blend)
{
    const Vec3F translation = maths::mix(fighter.previous.translation, fighter.current.translation, blend);
    const QuatF rotation = maths::slerp(fighter.previous.rotation, fighter.current.rotation, blend);
    const Mat4F modelMatrix = maths::transform(translation, rotation);

    const sq::Armature::Pose pose = fighter.get_armature().blend_poses(fighter.previous.pose, fighter.current.pose, blend);

    SkellyBlock block;

    block.matrix = renderer.get_camera().get_combo_matrix() * modelMatrix;
    block.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_view_matrix() * modelMatrix));

    fighter.get_armature().compute_ubo_data(pose, block.bones, 80u);

    mUbo.update(0u, uint(sizeof(StaticBlock) + sizeof(Mat34F) * pose.size()), &block);

    mConditionFlinch = fighter.should_render_flinch_models();
}

//============================================================================//

void RenderFighter::render_opaque() const
{
    renderer.context.bind_buffer(mUbo, sq::BufTarget::Uniform, 2u);
    base_render_opaque();
}

//============================================================================//

void RenderFighter::render_transparent() const
{
}
