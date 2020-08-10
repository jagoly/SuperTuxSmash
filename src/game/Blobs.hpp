#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

// some notes about HitBlobs and HurtBlobs:
// - the group value is equivilant to the Part value in smash bros
// - origin is always relative to the object, even when attached to a bone

//============================================================================//

/// doesn't do anything except pick the debug colour
enum class BlobFlavour : int8_t { Sour, Tangy, Sweet };

/// the order of this matters as it's used for priority
enum class BlobRegion : int8_t { Middle, Lower, Upper };

//============================================================================//

struct alignas(16) HitBlob final
{
    Fighter* fighter; ///< The fighter who owns this blob.
    Action* action;   ///< The action that created this blob.

    Vec3F origin; ///< Local origin of the blob sphere.
    float radius; ///< Local radius of the blob sphere.

    maths::Sphere sphere; ///< Blob sphere after transform.

    int8_t bone; ///< Index of the bone to attach to.

    uint8_t group; ///< Groups may only collide once per fighter per action.
    uint8_t index; ///< Used to choose from blobs from the same group.

    float damage;       ///< How much damage the blob will do when hit.
    float knockAngle;   ///< Angle of knockback in degrees (0 = forward, 90 = up).
    float knockBase;    ///< Base knockback to apply on collision.
    float knockScale;   ///< Scale the knockback based on current fighter damage.
    float freezeFactor; ///< Multiplier for the amount of freeze frames caused.

    BlobFlavour flavour; ///< Flavour of blob from sour (worst) to sweet (best).

    //TinyString name; ///< Name of the blob, used for debug and the edtior.

    constexpr Vec3F get_debug_colour() const
    {
        SWITCH (flavour) {
        CASE (Sour)  return { 0.6f, 0.6f, 0.0f };
        CASE (Tangy) return { 0.2f, 1.0f, 0.0f };
        CASE (Sweet) return { 1.0f, 0.1f, 0.1f };
        } SWITCH_END;
        return {}; // gcc warns without this
    }

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

struct alignas(16) HurtBlob final
{
    Fighter* fighter; ///< The fighter who owns this blob.

    Vec3F originA; ///< Local first origin of the blob capsule.
    Vec3F originB; ///< Local second origin of the blob capsule.
    float radius;  ///< Local radius of the blob capsule.

    maths::Capsule capsule; ///< Blob capsule after transform.

    int8_t bone; ///< Index of the bone to attach to.

    BlobRegion region; ///< What animations should this trigger.

    //TinyString name; ///< Name of the blob, used for debug and the edtior.

    constexpr Vec3F get_debug_colour() const
    {
        SWITCH (region) {
        CASE (Lower)  return { 0.8f, 0.0f, 1.0f };
        CASE (Middle) return { 0.4f, 0.4f, 1.0f };
        CASE (Upper)  return { 0.0f, 0.8f, 1.0f };
        } SWITCH_END;
        return {}; // gcc warns without this
    }

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

static_assert(sizeof(HitBlob) == 80u);
static_assert(sizeof(HurtBlob) == 80u);
//static_assert(sizeof(HitBlob) == 96u);
//static_assert(sizeof(HurtBlob) == 96u);

bool operator==(const HitBlob& a, const HitBlob& b);
bool operator==(const HurtBlob& a, const HurtBlob& b);

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobFlavour, Sour, Tangy, Sweet)
SQEE_ENUM_HELPER(sts::BlobRegion, Middle, Lower, Upper)
