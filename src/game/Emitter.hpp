#pragma once

#include "setup.hpp"

#include <sqee/maths/Random.hpp>
#include <sqee/misc/StackVector.hpp>

namespace sts {

//============================================================================//

struct Emitter final
{
    Fighter* fighter = nullptr; ///< The fighter who owns this emitter.
    Action* action = nullptr;   ///< The action that created this emitter.

    //--------------------------------------------------------//

    int8_t bone = -1; ///< Index of the bone to attach to.

    Vec3F origin;    ///< Relative origin of the generated shape.
    Vec3F velocity;  ///< Relative velocity of the generated shape.

    TinyString sprite; ///< Sprite to use.

    sq::StackVector<Vec3F, 8> colour; ///< Random colour choices.

    float baseOpacity = 1.f; ///< Starting opacity.
    float endOpacity = 0.f;  ///< End of life opacity factor.
    float endScale = 1.f;    ///< End of life scale factor.

    maths::RandomRange<uint16_t> lifetime;   ///< Random particle lifetime.
    maths::RandomRange<float>    baseRadius; ///< Random starting radius.

    //--------------------------------------------------------//

    maths::RandomRange<float> ballOffset; ///< Random ball starting offset.
    maths::RandomRange<float> ballSpeed;  ///< Random ball launch speed.

    maths::RandomRange<float> discIncline; ///< Random disc launch incline.
    maths::RandomRange<float> discOffset;  ///< Random disc starting offset.
    maths::RandomRange<float> discSpeed;   ///< Random disc launch speed.

    //--------------------------------------------------------//

    /// Generate count particles into a system.
    void generate(ParticleSystem& system, uint count);

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;

    //-- static members / methods ----------------------------//

    static void reset_random_seed(uint_fast32_t seed);
};

//============================================================================//

static_assert(sizeof(Emitter) == 224u);

bool operator==(const Emitter& a, const Emitter& b);

//============================================================================//

} // namespace sts
