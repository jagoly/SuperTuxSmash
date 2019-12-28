#pragma once

#include <sqee/macros.hpp>

#include <sqee/misc/Json.hpp>
#include <sqee/misc/TinyString.hpp>
#include <sqee/maths/Volumes.hpp>

#include "game/forward.hpp"

namespace sts {

//============================================================================//

constexpr const float BLOB_RADIUS_MIN = 0.05f;
constexpr const float BLOB_RADIUS_MAX = 2.f;
constexpr const float BLOB_DAMAGE_MIN = 0.1f;
constexpr const float BLOB_DAMAGE_MAX = 50.f;
constexpr const float BLOB_KNOCK_MIN = 0.f;
constexpr const float BLOB_KNOCK_MAX = 100.f;

//============================================================================//

enum class BlobFlavour : char { Sour, Tangy, Sweet };

enum class BlobPriority : char { Low, Normal, High, Transcend };

//============================================================================//

struct alignas(16) HitBlob final
{
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

    BlobFlavour flavour;   ///< Flavour of blob from sour (worst) to sweet (best).
    BlobPriority priority; ///< Priority of blob when colliding with other hit blobs.

    float damage;     ///< How much damage the blob will do when hit.
    float knockAngle; ///< Angle of knockback in cycles (0.0 == up, 0.25 == xdir, 0.5 == down)
    float knockBase;  ///< Base knockback to apply on collision.
    float knockScale; ///< Scale the knockback based on current fighter damage.

    //--------------------------------------------------------//

    constexpr Vec3F get_debug_colour() const
    {
        SWITCH ( flavour ) {
        CASE ( Sour )  return { 0.6f, 0.6f, 0.0f };
        CASE ( Tangy ) return { 0.2f, 1.0f, 0.0f };
        CASE ( Sweet ) return { 1.0f, 0.5f, 0.5f };
        } SWITCH_END;

        return {}; // gcc warns without this
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

static_assert(std::is_trivially_copyable_v<HitBlob> == true);
static_assert(std::is_trivially_copyable_v<HurtBlob> == true);

//============================================================================//

bool operator==(const HitBlob& a, const HitBlob& b);
bool operator==(const HurtBlob& a, const HurtBlob& b);

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::BlobFlavour, Sour, Tangy, Sweet)
SQEE_ENUM_HELPER(sts::BlobPriority, Low, Normal, High, Transcend)

SQEE_ENUM_JSON_CONVERSIONS(sts::BlobFlavour)
SQEE_ENUM_JSON_CONVERSIONS(sts::BlobPriority)
