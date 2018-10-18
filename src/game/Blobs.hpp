#pragma once

#include <sqee/macros.hpp>

#include <sqee/misc/Json.hpp>
#include <sqee/misc/TinyString.hpp>
#include <sqee/maths/Volumes.hpp>

#include "game/forward.hpp"

namespace sts {

//============================================================================//

struct alignas(16) HitBlob final
{
    enum class Flavour : char { Sour, Tangy, Sweet };

    enum class Priority : char { Low, Normal, High, Transcend };

    //--------------------------------------------------------//

    HitBlob() = default;

    HitBlob(const HitBlob& other) = default;
    HitBlob& operator=(const HitBlob& other) = default;

    //--------------------------------------------------------//

    Fighter* fighter; ///< The fighter who owns this blob.
    Action* action;   ///< The action that created this blob.

    Vec3F origin; ///< Local origin of the blob sphere.
    float radius; ///< Local radius of the blob sphere.

    sq::maths::Sphere sphere; ///< Blob sphere after transform.

    int8_t bone; ///< Index of the bone to attach to.

    //--------------------------------------------------------//

    uint8_t group; ///< Groups may only collide once per fighter per action.

    Flavour flavour;   ///< Flavour of blob from sour (worst) to sweet (best).
    Priority priority; ///< Priority of blob when colliding with other hit blobs.

    float damage;     ///< How much damage the blob will do when hit.
    float knockAngle; ///< Angle of knockback in turns (0.0 == up, 0.25 == xdir, 0.5 == down)
    float knockBase;  ///< Base knockback to apply on collision.
    float knockScale; ///< Scale the knockback based on current fighter damage.

    //--------------------------------------------------------//

    constexpr Vec3F get_debug_colour() const
    {
        SWITCH ( flavour ) {
        CASE ( Sour )  return { 1.0f, 1.0f, 0.0f };
        CASE ( Tangy ) return { 1.0f, 1.0f, 0.0f };
        CASE ( Sweet ) return { 1.0f, 0.6f, 0.6f };
        } SWITCH_END; return {};
    }

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

struct alignas(16) HurtBlob final
{
    HurtBlob(Fighter& fighter) : fighter(&fighter) {}

    //--------------------------------------------------------//

    Fighter* fighter; ///< The fighter who owns this blob.

    Vec3F originA; ///< Local first origin of the blob capsule.
    Vec3F originB; ///< Local second origin of the blob capsule.
    float radius;  ///< Local radius of the blob capsule.

    sq::maths::Capsule capsule; ///< Blob capsule after transform.

    int8_t bone; ///< Index of the bone to attach to.

    //--------------------------------------------------------//

    constexpr Vec3F get_debug_colour() const
    {
        return { 0.5f, 0.5f, 1.0f };
    }

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

static_assert(sizeof(HitBlob) == 80u);

static_assert(sizeof(HurtBlob) == 80u);

//============================================================================//

SQEE_ENUM_TO_STRING(HitBlob::Flavour, Sour, Tangy, Sweet)
SQEE_ENUM_TO_STRING(HitBlob::Priority, Low, Normal, High, Transcend)

SQEE_ENUM_JSON_CONVERSION_DECLARATIONS(HitBlob::Flavour)
SQEE_ENUM_JSON_CONVERSION_DECLARATIONS(HitBlob::Priority)

//============================================================================//

} // namespace sts
