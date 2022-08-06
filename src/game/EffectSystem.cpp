#include "game/EffectSystem.hpp"

#include "game/Entity.hpp"
#include "game/VisualEffect.hpp"

#include "render/Renderer.hpp"

using namespace sts;

//============================================================================//

EffectSystem::EffectSystem(Renderer& renderer) : renderer(renderer) {}

EffectSystem::~EffectSystem() = default;

//============================================================================//

int32_t EffectSystem::play_effect(const VisualEffectDef& def, const Entity* owner)
{
    const EffectAsset& asset = def.handle.get();
    VisualEffect& effect = *mEffects.emplace_back(std::make_unique<VisualEffect>(def, owner));

    if (def.attached == false)
    {
        if (owner == nullptr)
        {
            effect.modelMatrix = def.localMatrix;
            effect.bbScaleX = 1.f;
        }
        else
        {
            effect.modelMatrix = owner->get_bone_matrix(def.bone) * def.localMatrix;
            effect.bbScaleX = float(owner->get_vars().facing);
        }
    }
    else SQASSERT(owner != nullptr, "cannot attach to null owner");

    // compute sample for frame zero
    asset.armature.compute_sample(asset.animation, 0.f, effect.animPlayer.currentSample);

    effect.animPlayer.animTime = 1.f;

    // assign the effect a unique id and return it
    return effect.id = ++mCurrentId;
}

//============================================================================//

void EffectSystem::cancel_effect(int32_t id)
{
    const auto iter = algo::find_if(mEffects, [&](const auto& effect){ return effect->id == id; });
    if (iter != mEffects.end()) mEffects.erase(iter);
}

//============================================================================//

void EffectSystem::clear()
{
    mEffects.clear();
}

//============================================================================//

void EffectSystem::tick()
{
    for (auto iter = mEffects.begin(); iter != mEffects.end();)
    {
        VisualEffect& effect = **iter;
        const EffectAsset& asset = effect.def.handle.get();

        if (uint(effect.animPlayer.animTime) == asset.animation.frameCount)
        {
            mEffects.erase(iter);
            continue;
        }

        std::swap(effect.animPlayer.previousSample, effect.animPlayer.currentSample);

        asset.armature.compute_sample(asset.animation, effect.animPlayer.animTime, effect.animPlayer.currentSample);

        effect.animPlayer.animTime += 1.f;
        ++iter;
    }
}

//============================================================================//

void EffectSystem::integrate(float blend)
{
    for (auto iter = mEffects.begin(); iter != mEffects.end(); ++iter)
    {
        VisualEffect& effect = **iter;
        const EffectAsset& asset = effect.def.handle.get();

        if (effect.def.attached == true)
        {
            effect.modelMatrix = effect.entity->get_blended_bone_matrix(effect.def.bone) * effect.def.localMatrix;
            effect.bbScaleX = float(effect.entity->get_vars().facing);
        }

        effect.animPlayer.integrate(renderer, effect.modelMatrix, effect.bbScaleX, blend);

        const auto check_condition = [&](const TinyString& condition)
        {
            // todo: do effects need conditions?
            //       should probably use the conditions of the entity that spawned them
            if (condition.empty()) return true;
            SQASSERT(false, "invalid condition");
        };

        for (const sq::DrawItem& item : asset.drawItems)
            if (check_condition(item.condition) == true)
                renderer.add_draw_call(item, effect.animPlayer);
    }
}
