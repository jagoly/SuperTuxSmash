#pragma once

#include "setup.hpp"

#include <sqee/maths/Random.hpp>
#include <sqee/misc/StackVector.hpp>

#include <variant>

namespace sts {

//============================================================================//

struct Emitter final
{
    Vec3F emitPosition; ///< World position of the emitter.
    Vec3F emitVelocity; ///< Velocity of the emitter.

    Fighter* fighter; ///< The fighter who owns this emitter.
    Action* action;   ///< The action that created this emitter.

    //--------------------------------------------------------//

    int8_t bone = -1; ///< Index of the bone to attach to.

    Vec3F origin;    ///< Relative origin of the generated shape.
    Vec3F direction; ///< Relative direction of the generated shape.

    TinyString sprite; ///< Sprite to use.

    float endScale;   ///< End of life scale factor.
    float endOpacity; ///< End of life opacity factor.

    maths::RandomRange<uint16_t> lifetime; ///< Random lifetime.
    maths::RandomRange<float>    radius;   ///< Random radius.
    maths::RandomRange<float>    opacity;  ///< Random opacity.
    maths::RandomRange<float>    speed;    ///< Random shapeless launch speed.

    //--------------------------------------------------------//

    /// Always use a fixed colour.
    using FixedColour = Vec3F;

    /// Use one of multiple possible colours.
    using RandomColour = sq::StackVector<Vec3F, 8u>;

    std::variant<FixedColour, RandomColour> colour;

    FixedColour& colour_fixed() { return std::get<FixedColour>(colour); }
    RandomColour& colour_random() { return std::get<RandomColour>(colour); }

    //--------------------------------------------------------//

    /// Spawn particles at origin, and launch them in every direction to form a ball.
    struct BallShape
    {
        maths::RandomRange<float> speed;  ///< Random ball launch velocity.
    };

    /// Spawn particles at origin, and launch them outwards to form a disc.
    struct DiscShape
    {
        maths::RandomRange<float> incline; ///< Random incline for launch direction.
        maths::RandomRange<float> speed;   ///< Random disc launch velocity.
    };

    std::variant<BallShape, DiscShape> shape;

    BallShape& shape_ball() { return std::get<BallShape>(shape); }
    DiscShape& shape_disc() { return std::get<DiscShape>(shape); }

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

static_assert(sizeof(Emitter) <= 256u);

bool operator==(const Emitter& a, const Emitter& b);

//============================================================================//

} // namespace sts
