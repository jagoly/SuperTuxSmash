#pragma once

#include <variant>

#include <sqee/misc/Json.hpp>

#include "game/forward.hpp"
#include "game/ParticleSystem.hpp"

namespace sts {

//============================================================================//

struct ParticleEmitter final
{
    Vec3F origin; ///< Relative origin of the emitter.

    Fighter* fighter; ///< The fighter who owns this emitter.
    Action* action;   ///< The action that created this emitter.

    int8_t bone = -1; ///< Index of the bone to attach to.

    //--------------------------------------------------------//

    Vec3F emitPosition; ///< Base position for all particles.
    Vec3F emitVelocity; ///< Base velocity for all particles.

    Vec3F direction; ///< Direction of the generated shape.

    TinyString sprite; ///< Sprite to use.

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

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

} // namespace sts
