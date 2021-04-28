#include "game/EffectSystem.hpp"

#include "game/Fighter.hpp"
#include "game/VisualEffect.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"

#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

EffectSystem::EffectSystem(Renderer& renderer) : renderer(renderer)
{

}

//============================================================================//

void EffectSystem::play_effect(const VisualEffect& effect)
{
    ActiveEffect& instance = mActiveEffects.emplace_back();

    instance.effect = &effect;
    instance.frame = 1u;

    // todo: give EffectSystem a pool of permanent ubos that get given out to
    // effects when they are played, rather than creating new ones all the time
    instance.ubo = std::make_unique<sq::FixedBuffer>();
    instance.ubo->allocate_dynamic(sizeof(EffectBlock));

    const EffectAsset& asset = effect.handle.get();
    const Fighter& fighter = *effect.fighter;

    if (effect.anchored == false)
        instance.worldMatrix = fighter.get_model_matrix() * effect.localMatrix;

    instance.current.pose = asset.armature.compute_pose_discrete(asset.animation, 0u);

    for (uint i = 0u; i < asset.animation.boneCount; ++i)
        if (asset.paramTracks[i] != nullptr)
            asset.armature.compute_extra_discrete(instance.current.params[i], *asset.paramTracks[i], 0u);

    //instance.renderGroupId = renderer.create_draw_items(asset.drawItemDefs, instance.ubo.get(), {});
}

//============================================================================//

void EffectSystem::cancel_effect(const VisualEffect& /*effect*/)
{
    // todo
}

//============================================================================//

void EffectSystem::clear()
{
    for (ActiveEffect& instance : mActiveEffects)
        renderer.delete_draw_items(instance.renderGroupId);

    mActiveEffects.clear();
}

//============================================================================//

void EffectSystem::tick()
{
    //-- delete finished instances and draw items ------------//

    for (ActiveEffect& instance : mActiveEffects)
        if (instance.frame == instance.effect->handle->animation.frameCount)
            renderer.delete_draw_items(instance.renderGroupId);

    const auto predicate = [](ActiveEffect& item) { return item.frame == item.effect->handle->animation.frameCount; };
    algo::erase_if(mActiveEffects, predicate);

    //-- perform the actual updates --------------------------//

    for (ActiveEffect& instance : mActiveEffects)
    {
        const EffectAsset& asset = instance.effect->handle.get();

        instance.previous = instance.current;

        instance.current.pose = asset.armature.compute_pose_discrete(asset.animation, instance.frame);

        for (uint i = 0u; i < asset.animation.boneCount; ++i)
            if (asset.paramTracks[i] != nullptr)
                asset.armature.compute_extra_discrete(instance.current.params[i], *asset.paramTracks[i], instance.frame);

        instance.frame += 1u;
    }
}

//============================================================================//

void EffectSystem::integrate(float blend)
{
    for (ActiveEffect& instance : mActiveEffects)
    {
        const VisualEffect& effect = *instance.effect;
        const EffectAsset& asset = effect.handle.get();
        const Fighter& fighter = *effect.fighter;

        // todo: allow anchoring to specific bones, like with hitblobs, emitters, etc
        const Mat4F modelMatrix = effect.anchored ? fighter.get_interpolated_model_matrix() * effect.localMatrix
                                                  : instance.worldMatrix;

        const sq::Armature::Pose pose = asset.armature.blend_poses(instance.previous.pose, instance.current.pose, blend);

        EffectBlock block;

        block.matrix = renderer.get_camera().get_block().projViewMat * modelMatrix;
        block.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_block().viewMat * modelMatrix));

        asset.armature.compute_ubo_data(pose, block.bones, 8u);

        for (uint i = 0u; i < asset.animation.boneCount; ++i)
            if (asset.paramTracks[i] != nullptr)
                block.params[i] = maths::mix(instance.previous.params[i], instance.current.params[i], blend);

        instance.ubo->update(0u, block);
    }
}
