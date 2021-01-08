#pragma once

#include "setup.hpp"

#include <sqee/gl/FixedBuffer.hpp>
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

    struct ActiveEffect
    {
        const VisualEffect* effect;
        std::unique_ptr<sq::FixedBuffer> ubo;
        uint frame;        
        int64_t renderGroupId;
        Mat4F worldMatrix;

        struct InterpolationData
        {
            sq::Armature::Pose pose;
            Vec4F params[MAX_EFFECT_BONES] {};
        }
        previous, current;
    };

    std::vector<ActiveEffect> mActiveEffects;
};

//============================================================================//

} // namespace sts
