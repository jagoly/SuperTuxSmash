#pragma once

#include <variant>

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>
#include <sqee/misc/Json.hpp>

#include "game/ParticleSystem.hpp"

namespace sts {

//============================================================================//

struct ParticleEmitter final : sq::NonCopyable
{
    Vec3F origin; ///< Relative origin of the emitter.

    int8_t bone = -1; ///< Index of the bone to attach to.

    //--------------------------------------------------------//

    Vec3F emitPosition; ///< Base position for all particles.
    Vec3F emitVelocity; ///< Base velocity for all particles.

    Vec3F direction; ///< Direction of the generated shape.

    struct { uint16_t min, max; } sprite;   ///< Random sprite.
    struct { uint16_t min, max; } lifetime; ///< Random lifetime.
    struct { float    min, max; } scale;    ///< Random radius scale.

    struct { float start, end; } radius;  ///< Particle radius.
    struct { float start, end; } opacity; ///< Particle opacity.

    struct DiscShape
    {
        struct { float min, max; } incline; ///< Random incline for launch direction.
        struct { float min, max; } speed;   ///< Random magnitude for launch velocity.
        float _padding[4];
    };

    struct ColumnShape
    {
        struct { float min, max; } deviation; ///< Random distance from column centre.
        struct { float min, max; } speed;     ///< Random magnitude for launch velocity.
        float _padding[4];
    };

    struct BallShape
    {
        struct { float min, max; } speed;
        float _padding[6];
    };

    std::variant<DiscShape, ColumnShape, BallShape> shape;

    DiscShape& disc() { return std::get<DiscShape>(shape); }
    ColumnShape& column() { return std::get<ColumnShape>(shape); }
    BallShape& ball() { return std::get<BallShape>(shape); }

    /// Generate count particles into a system.
    void generate(ParticleSystem& system, uint count);

    void load_from_json(ParticleSystem& system, const sq::JsonValue& json);
};

//============================================================================//

} // namespace sts
