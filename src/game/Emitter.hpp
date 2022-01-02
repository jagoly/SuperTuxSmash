#pragma once

#include "setup.hpp"

#include <sqee/maths/Random.hpp>

namespace sts {

//============================================================================//

struct Emitter final
{
    Fighter* fighter = nullptr; ///< Fighter that owns this emitter.

    //--------------------------------------------------------//

    int8_t bone = -1; ///< Index of the bone to attach to.

    uint8_t count = 0u; ///< Number of particles to emit at a time.

    Vec3F origin   = {}; ///< Relative origin of the generated shape.
    Vec3F velocity = {}; ///< Relative velocity of the generated shape.

    float baseOpacity = 1.f; ///< Starting opacity.
    float endOpacity  = 0.f; ///< End of life opacity factor.
    float endScale    = 1.f; ///< End of life scale factor.

    maths::RandomRange<uint16_t> lifetime   = {}; ///< Random particle lifetime.
    maths::RandomRange<float>    baseRadius = {}; ///< Random starting radius.

    maths::RandomRange<float> ballOffset = {}; ///< Random ball starting offset.
    maths::RandomRange<float> ballSpeed  = {}; ///< Random ball launch speed.

    maths::RandomRange<float> discIncline = {}; ///< Random disc launch incline.
    maths::RandomRange<float> discOffset  = {}; ///< Random disc starting offset.
    maths::RandomRange<float> discSpeed   = {}; ///< Random disc launch speed.

    StackVector<Vec3F, 8> colour = {}; ///< Random colour choices.

    TinyString sprite = {}; ///< Sprite array to use.

    //--------------------------------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    void from_json(const JsonValue& json);

    void to_json(JsonValue& json) const;

    bool operator==(const Emitter& other) const;
};

//============================================================================//

} // namespace sts
