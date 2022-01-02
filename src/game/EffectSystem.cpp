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
    const auto& ctx = sq::VulkanContext::get();

    for (uint8_t i = 0u; i < MAX_ACTIVE_EFFECTS; ++i)
    {
        BufferPoolEntry& entry = mBufferPool[i];

        entry.ubo.initialise(sizeof(EffectBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        entry.descriptorSet = sq::vk_allocate_descriptor_set_swapper(ctx, renderer.setLayouts.object);

        sq::vk_update_descriptor_set_swapper (
            ctx, entry.descriptorSet, sq::DescriptorUniformBuffer(0u, 0u, entry.ubo.get_descriptor_info())
        );

        mBufferPoolSorted[i] = &entry;
    }
}

//============================================================================//

void EffectSystem::play_effect(const VisualEffect& effect)
{
    // find the entry that has been unused for the longest time
    auto iter = algo::find_if(mBufferPoolSorted, [](BufferPoolEntry* item){ return item->used == false; });
    if (iter == mBufferPoolSorted.end())
    {
        sq::log_warning("too many active effects");
        return;
    }

    ActiveEffect& instance = mActiveEffects.emplace_back();
    const EffectAsset& asset = effect.handle.get();
    const Fighter& fighter = *effect.fighter;

    instance.effect = &effect;

    // mark entry as used and move it to the end
    instance.entry = *iter;
    std::copy(iter + 1, mBufferPoolSorted.end(), iter);
    mBufferPoolSorted.back() = instance.entry;
    instance.entry->used = true;

    instance.renderGroupId = renderer.create_draw_items(asset.drawItemDefs, instance.entry->descriptorSet, {});

    if (effect.anchored == false)
        instance.modelMatrix = fighter.get_bone_matrix(effect.bone) * effect.localMatrix;

    // set pose and params for frame zero (used as previous)
    instance.current.pose = asset.armature.compute_pose_discrete(asset.animation, 0u);
    for (uint i = 0u; i < asset.animation.boneCount; ++i)
        if (asset.paramTracks[i] != nullptr)
            asset.armature.compute_extra_discrete(instance.current.params[i], *asset.paramTracks[i], 0u);

    instance.frame = 1u;
}

//============================================================================//

void EffectSystem::cancel_effect(const VisualEffect& effect)
{
    auto iter = algo::find_if(mActiveEffects, [&](ActiveEffect& item){ return item.effect == &effect; });
    if (iter != mActiveEffects.end())
    {
        iter->entry->used = false;
        renderer.delete_draw_items(iter->renderGroupId);
        mActiveEffects.erase(iter);
    }
}

//============================================================================//

void EffectSystem::clear()
{
    for (ActiveEffect& instance : mActiveEffects)
        renderer.delete_draw_items(instance.renderGroupId);

    for (uint8_t i = 0u; i < MAX_ACTIVE_EFFECTS; ++i)
    {
        mBufferPool[i].used = false;
        mBufferPoolSorted[i] = &mBufferPool[i];
    }

    mActiveEffects.clear();
}

//============================================================================//

void EffectSystem::tick()
{
    for (auto iter = mActiveEffects.begin(); iter != mActiveEffects.end();)
    {
        ActiveEffect& instance = *iter;
        const EffectAsset& asset = instance.effect->handle.get();

        // animation finished, erase the instance
        if (instance.frame == asset.animation.frameCount)
        {
            instance.entry->used = false;
            renderer.delete_draw_items(instance.renderGroupId);
            mActiveEffects.erase(iter);
            continue;
        }

        instance.previous = instance.current;

        instance.current.pose = asset.armature.compute_pose_discrete(asset.animation, instance.frame);
        for (uint i = 0u; i < asset.animation.boneCount; ++i)
            if (asset.paramTracks[i] != nullptr)
                asset.armature.compute_extra_discrete(instance.current.params[i], *asset.paramTracks[i], instance.frame);

        ++instance.frame; ++iter;
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

        if (effect.anchored == true)
            instance.modelMatrix = fighter.get_blended_bone_matrix(effect.bone) * effect.localMatrix;

        const auto pose = asset.armature.blend_poses(instance.previous.pose, instance.current.pose, blend);

        instance.entry->descriptorSet.swap();
        auto& block = *reinterpret_cast<EffectBlock*>(instance.entry->ubo.swap_map());

        block.matrix = renderer.get_camera().get_block().projViewMat * instance.modelMatrix;
        block.normMat = Mat34F(maths::normal_matrix(renderer.get_camera().get_block().viewMat * instance.modelMatrix));

        asset.armature.compute_ubo_data(pose, block.bones, 8u);

        for (uint i = 0u; i < asset.animation.boneCount; ++i)
            if (asset.paramTracks[i] != nullptr)
                block.params[i] = maths::mix(instance.previous.params[i], instance.current.params[i], blend);
    }
}
