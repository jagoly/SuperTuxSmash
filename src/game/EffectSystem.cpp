#include "game/EffectSystem.hpp"

#include "game/Fighter.hpp"
#include "game/VisualEffect.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"

#include <sqee/maths/Functions.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

EffectSystem::EffectSystem(Renderer& renderer) : renderer(renderer)
{
    // todo: can be removed once pointers are removed from DrawItem
    mActiveEffects.reserve(128);
}

//============================================================================//

void EffectSystem::play_effect(const VisualEffect& effect)
{
    const auto& ctx = sq::VulkanContext::get();

    ActiveEffect& instance = mActiveEffects.emplace_back();

    instance.effect = &effect;
    instance.frame = 1u;

    // todo: give EffectSystem a pool of permanent ubos that get given out to
    // effects when they are played, rather than creating new ones all the time
    instance.ubo.initialise(sizeof(EffectBlock), vk::BufferUsageFlagBits::eUniformBuffer);
    instance.descriptorSet = sq::vk_allocate_descriptor_set_swapper(ctx, renderer.setLayouts.object);

    sq::vk_update_descriptor_set_swapper (
        ctx, instance.descriptorSet, 0u, 0u, vk::DescriptorType::eUniformBuffer,
        instance.ubo.get_descriptor_info_front(), instance.ubo.get_descriptor_info_back()
    );

    const EffectAsset& asset = effect.handle.get();
    const Fighter& fighter = *effect.fighter;

    if (effect.anchored == false)
        instance.worldMatrix = fighter.get_model_matrix() * effect.localMatrix;

    instance.current.pose = asset.armature.compute_pose_discrete(asset.animation, 0u);

    for (uint i = 0u; i < asset.animation.boneCount; ++i)
        if (asset.paramTracks[i] != nullptr)
            asset.armature.compute_extra_discrete(instance.current.params[i], *asset.paramTracks[i], 0u);

    instance.renderGroupId = renderer.create_draw_items(asset.drawItemDefs, instance.descriptorSet, {});
}

//============================================================================//

void EffectSystem::cancel_effect(const VisualEffect& /*effect*/)
{
    // todo
}

//============================================================================//

void EffectSystem::clear()
{
    const auto& ctx = sq::VulkanContext::get();

    for (ActiveEffect& instance : mActiveEffects)
    {
        renderer.delete_draw_items(instance.renderGroupId);
        ctx.device.free(ctx.descriptorPool, {instance.descriptorSet.front, instance.descriptorSet.back});
    }

    mActiveEffects.clear();
}

//============================================================================//

void EffectSystem::tick()
{
    const auto& ctx = sq::VulkanContext::get();

    //-- delete finished instances and free resources --------//

    const auto predicate = [&ctx](ActiveEffect& instance)
    {
        // wait a few extra frames to ensure resources are no longer in use
        if (instance.frame == instance.effect->handle->animation.frameCount + 4u)
        {
            ctx.device.free(ctx.descriptorPool, {instance.descriptorSet.front, instance.descriptorSet.back});
            return true;
        }
        return false;
    };
    algo::erase_if(mActiveEffects, predicate);

    //-- perform the actual updates --------------------------//

    for (ActiveEffect& instance : mActiveEffects)
    {
        const EffectAsset& asset = instance.effect->handle.get();

        if (instance.frame < asset.animation.frameCount)
        {
            instance.previous = instance.current;

            instance.current.pose = asset.armature.compute_pose_discrete(asset.animation, instance.frame);

            for (uint i = 0u; i < asset.animation.boneCount; ++i)
                if (asset.paramTracks[i] != nullptr)
                    asset.armature.compute_extra_discrete(instance.current.params[i], *asset.paramTracks[i], instance.frame);
        }
        else renderer.delete_draw_items(instance.renderGroupId);

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

        // instance is done and waiting for deletion
        if (instance.frame > asset.animation.frameCount)
            continue;

        // todo: allow anchoring to specific bones, like with hitblobs, emitters, etc
        const Mat4F modelMatrix = effect.anchored ? fighter.get_interpolated_model_matrix() * effect.localMatrix
                                                  : instance.worldMatrix;

        const sq::Armature::Pose pose = asset.armature.blend_poses(instance.previous.pose, instance.current.pose, blend);

        instance.descriptorSet.swap();
        auto& block = *reinterpret_cast<EffectBlock*>(instance.ubo.swap_map());

        block.matrix = renderer.get_camera().get_block().projViewMat * modelMatrix;
        block.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_block().viewMat * modelMatrix));

        asset.armature.compute_ubo_data(pose, block.bones, 8u);

        for (uint i = 0u; i < asset.animation.boneCount; ++i)
            if (asset.paramTracks[i] != nullptr)
                block.params[i] = maths::mix(instance.previous.params[i], instance.current.params[i], blend);
    }
}
