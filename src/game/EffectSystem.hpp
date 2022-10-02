#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

class EffectSystem final
{
public: //====================================================//

    EffectSystem(Renderer& renderer);

    SQEE_COPY_DELETE(EffectSystem)
    SQEE_MOVE_DELETE(EffectSystem)

    ~EffectSystem();

    //--------------------------------------------------------//

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    int32_t play_effect(const VisualEffectDef& def, const Entity* owner);

    void cancel_effect(int32_t id);

    void clear();

private: //===================================================//

    Renderer& renderer;

    std::vector<std::unique_ptr<VisualEffect>> mEffects;

    int32_t mCurrentId = -1;
};

//============================================================================//

} // namespace sts
