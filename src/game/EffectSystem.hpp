#pragma once

#include "setup.hpp"

#include <sqee/vk/SwapBuffer.hpp>
#include <sqee/objects/Armature.hpp>

namespace sts {

//============================================================================//

class EffectSystem final : sq::NonCopyable
{
public: //====================================================//

    EffectSystem(Renderer& renderer);

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    void play_effect(const VisualEffect& effect);

    void cancel_effect(const VisualEffect& effect);

    void clear();

private: //===================================================//

    Renderer& renderer;

    struct BufferPoolEntry
    {
        sq::SwapBuffer ubo;
        sq::Swapper<vk::DescriptorSet> descriptorSet;
        bool used = false;
    };

    struct ActiveEffect
    {
        const VisualEffect* effect;
        BufferPoolEntry* entry;
        uint frame;
        int64_t renderGroupId;
        Mat4F modelMatrix;

        struct InterpolationData
        {
            sq::Armature::Pose pose;
            Vec4F params[MAX_EFFECT_BONES] {};
        }
        previous, current;
    };

    std::array<BufferPoolEntry, MAX_ACTIVE_EFFECTS> mBufferPool;
    std::array<BufferPoolEntry*, MAX_ACTIVE_EFFECTS> mBufferPoolSorted;

    StackVector<ActiveEffect, MAX_ACTIVE_EFFECTS> mActiveEffects;
};

//============================================================================//

} // namespace sts
