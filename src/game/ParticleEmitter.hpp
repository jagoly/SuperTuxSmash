#pragma once

#include <variant>

#include <sqee/misc/Json.hpp>
#include <sqee/misc/StaticVector.hpp>
#include <sqee/maths/Random.hpp>

#include "game/forward.hpp"
#include "game/ParticleSystem.hpp"

namespace sts {

//============================================================================//

constexpr const float EMIT_END_SCALE_MAX = 10.f;
constexpr const float EMIT_END_OPACITY_MAX = 10.f;
constexpr const uint16_t EMIT_RAND_LIFETIME_MIN = 4u;
constexpr const uint16_t EMIT_RAND_LIFETIME_MAX = 480u;
constexpr const float EMIT_RAND_RADIUS_MIN = 0.05f;
constexpr const float EMIT_RAND_RADIUS_MAX = 1.f;
constexpr const float EMIT_RAND_OPACITY_MIN = 0.1f;
constexpr const float EMIT_RAND_SPEED_MAX = 20.f;

//============================================================================//

using sq::maths::RandomRange;

//============================================================================//

struct ParticleEmitter final
{
    Vec3F emitPosition; ///< World position of the emitter.
    Vec3F emitVelocity; ///< Velocity of the emitter.

    Fighter* fighter; ///< The fighter who owns this emitter.
    Action* action;   ///< The action that created this emitter.

    //--------------------------------------------------------//

    int8_t bone = -1; ///< Index of the bone to attach to.

    Vec3F origin; ///< Relative origin of the generated shape.
    Vec3F direction; ///< Relative direction of the generated shape.

    TinyString sprite; ///< Sprite to use.

    float endScale;   ///< End of life scale factor.
    float endOpacity; ///< End of life opacity factor.

    RandomRange<uint16_t> lifetime; ///< Random lifetime.
    RandomRange<float>    radius;   ///< Random radius.
    RandomRange<float>    opacity;  ///< Random opacity.
    RandomRange<float>    speed;    ///< Random shapeless launch speed.

    //--------------------------------------------------------//

    /// Always use a fixed colour.
    using FixedColour = Vec3F;

    /// Use one of multiple possible colours.
    using RandomColour = sq::StaticVector<Vec3F, 8u>;

    std::variant<FixedColour, RandomColour> colour;

    FixedColour& colour_fixed() { return std::get<FixedColour>(colour); }
    RandomColour& colour_random() { return std::get<RandomColour>(colour); }

    //--------------------------------------------------------//

    /// Spawn particles at origin, and launch them in every direction to form a ball.
    struct BallShape
    {
        RandomRange<float> speed;  ///< Random ball launch velocity.
    };

    /// Spawn particles at origin, and launch them outwards to form a disc.
    struct DiscShape
    {
        RandomRange<float> incline; ///< Random incline for launch direction.
        RandomRange<float> speed;   ///< Random disc launch velocity.
    };

    /// Spawn particles in a ring around origin.
    struct RingShape
    {
        RandomRange<float> deviation; ///< Random distance from ring centre.
        RandomRange<float> incline;   ///< Random incline for deviation.
    };

    std::variant<BallShape, DiscShape, RingShape> shape;

    BallShape& shape_ball() { return std::get<BallShape>(shape); }
    DiscShape& shape_disc() { return std::get<DiscShape>(shape); }
    RingShape& shape_ring() { return std::get<RingShape>(shape); }

    //--------------------------------------------------------//

    /// Generate count particles into a system.
    void generate(ParticleSystem& system, uint count);

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;

    //-- static members / methods ----------------------------//

    static void reset_random_seed(uint64_t seed);
};

//============================================================================//

static_assert(sizeof(ParticleEmitter) <= 256u);

bool operator!=(const ParticleEmitter::BallShape& a, const ParticleEmitter::BallShape& b);
bool operator!=(const ParticleEmitter::DiscShape& a, const ParticleEmitter::DiscShape& b);
bool operator!=(const ParticleEmitter::RingShape& a, const ParticleEmitter::RingShape& b);

bool operator==(const ParticleEmitter& a, const ParticleEmitter& b);

//============================================================================//

} // namespace sts
