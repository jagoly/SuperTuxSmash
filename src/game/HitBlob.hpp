#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

/// special knockback angle calculation modes
enum class BlobAngleMode : int8_t { Normal, Sakurai, AutoLink };

/// how to choose the facing of the knockback
enum class BlobFacingMode : int8_t { Relative, Forward, Reverse };

/// what happens when the blob hits another hitblob
enum class BlobClangMode : int8_t { Ignore, Ground, Air, Cancel };

/// doesn't do anything except pick the debug colour
enum class BlobFlavour : int8_t { Sour, Tangy, Sweet };

//============================================================================//

struct HitBlob final
{
    FighterAction* action = nullptr; ///< Action that owns this blob.

    maths::Sphere sphere; ///< Blob sphere after transform.

    //--------------------------------------------------------//

    Vec3F origin; ///< Model space origin of the blob sphere.
    float radius; ///< Model space radius of the blob sphere.

    int8_t bone; ///< Index of the bone to attach to.

    uint8_t index; ///< Used to choose from blobs from the same group.

    float damage;       ///< How much damage the blob will do when hit.
    float freezeMult;   ///< Multiplier for https://www.ssbwiki.com/Hitlag
    float freezeDiMult; ///< Multiplier for https://www.ssbwiki.com/Smash_directional_influence

    float knockAngle;   ///< Knockback angle in degrees (0 = forward, 90 = up).
    float knockBase;    ///< Base knockback to apply on collision.
    float knockScale;   ///< Scale the knockback based on current fighter damage.

    BlobAngleMode  angleMode;  ///< https://www.ssbwiki.com/Angle#Special_angles
    BlobFacingMode facingMode; ///< https://www.ssbwiki.com/Angle#Angle_flipper
    BlobClangMode  clangMode;  ///< https://www.ssbwiki.com/Priority
    BlobFlavour    flavour;    ///< Flavour of blob from sour (worst) to sweet (best).

    bool ignoreDamage; ///< https://www.ssbwiki.com/Knockback#Set_knockback
    bool ignoreWeight; ///< https://www.ssbwiki.com/Knockback#Weight-independent_knockback

    bool canHitGround; ///< Can the blob hit targets on the ground.
    bool canHitAir;    ///< Can the blob hit targets in the air.

    TinyString handler; ///< Handler to use after collision.
    TinyString sound;   ///< Sound to play after collision.

    //--------------------------------------------------------//

    void from_json(const JsonValue& json);

    void to_json(JsonValue& json) const;

    Vec3F get_debug_colour() const;

    bool operator==(const HitBlob& other) const;

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobAngleMode, Normal, Sakurai, AutoLink)
SQEE_ENUM_HELPER(sts::BlobFacingMode, Relative, Forward, Reverse)
SQEE_ENUM_HELPER(sts::BlobClangMode, Ignore, Ground, Air, Cancel)
SQEE_ENUM_HELPER(sts::BlobFlavour, Sour, Tangy, Sweet)
