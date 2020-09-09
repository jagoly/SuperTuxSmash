#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

// - the group value is equivilant to the Part value in smash bros
// - origin is always relative to the fighter, even when the blob is attached to a bone
// - BlobFacing::Reverse needs to exist for backwards sakurai hits, otherwise just use an angle > 90

//============================================================================//

/// how to choose the facing of the knockback
enum class BlobFacing : int8_t { Relative, Forward, Reverse };

/// doesn't do anything except pick the debug colour
enum class BlobFlavour : int8_t { Sour, Tangy, Sweet };

//============================================================================//

//enum class BlobSound : int8_t
//{
//    None = -1,
//    PunchSmall, PunchMedium, PunchLarge
//};

//============================================================================//

struct alignas(16) HitBlob final
{
    Action* action = nullptr; ///< Action that owns this blob.

    maths::Sphere sphere; ///< Blob sphere after transform.

    //--------------------------------------------------------//

    Vec3F origin; ///< Local origin of the blob sphere.
    float radius; ///< Local radius of the blob sphere.

    int8_t bone; ///< Index of the bone to attach to.

    uint8_t group; ///< Groups may only collide once per fighter per action.
    uint8_t index; ///< Used to choose from blobs from the same group.

    float damage;       ///< How much damage the blob will do when hit.
    float freezeFactor; ///< Multiplier for the amount of freeze frames caused.
    float knockAngle;   ///< Angle of knockback in degrees (0 = forward, 90 = up).
    float knockBase;    ///< Base knockback to apply on collision.
    float knockScale;   ///< Scale the knockback based on current fighter damage.

    BlobFacing  facing;  ///< https://www.ssbwiki.com/Angle#Angle_flipper
    BlobFlavour flavour; ///< Flavour of blob from sour (worst) to sweet (best).

    bool useFixedKnockback; ///< https://www.ssbwiki.com/Knockback#Set_knockback
    bool useSakuraiAngle;   ///< https://www.ssbwiki.com/Sakurai_angle

    //-- debug / editor methods ------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    constexpr Vec3F get_debug_colour() const
    {
        SWITCH (flavour) {
        CASE (Sour)  return { 0.6f, 0.6f, 0.0f };
        CASE (Tangy) return { 0.2f, 1.0f, 0.0f };
        CASE (Sweet) return { 1.0f, 0.1f, 0.1f };
        CASE_DEFAULT throw std::exception();
        } SWITCH_END;
    }

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

bool operator==(const HitBlob& a, const HitBlob& b);

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobFacing, Relative, Forward, Reverse)
SQEE_ENUM_HELPER(sts::BlobFlavour, Sour, Tangy, Sweet)

//SQEE_ENUM_HELPER
//(
//    sts::BlobSound,
//    None,
//    PunchSmall, PunchMedium, PunchLarge
//)
