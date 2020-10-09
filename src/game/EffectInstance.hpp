#pragma once

#include "setup.hpp"

#include <assets/Effect.hpp>

namespace sts {

//============================================================================//

struct EffectInstance final
{
    Fighter* fighter = nullptr;
    int8_t bone = -1;

    EffectAssetHandle asset;
};

//============================================================================//

} // namespace sts
