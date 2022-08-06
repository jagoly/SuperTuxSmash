#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp>
#include <sqee/objects/Armature.hpp>

namespace sts {

//============================================================================//

/// Special knockback angle calculation modes.
enum class BlobAngleMode : int8_t { Normal, Sakurai, AutoLink };

/// How to choose the facing of the knockback.
enum class BlobFacingMode : int8_t { Relative, Forward, Reverse };

/// What happens when the blob hits another hitblob.
enum class BlobClangMode : int8_t { Ignore, Ground, Air, Cancel };

/// Doesn't affect anything except debug colour.
enum class BlobFlavour : int8_t { Sour, Tangy, Sweet };

//============================================================================//

struct HitBlobDef final
{
    Vec3F origin = {};  ///< Model space origin of the blob sphere.
    float radius = 0.f; ///< Model space radius of the blob sphere.

    float damage       = 0.f; ///< How much damage to do on collision.
    float freezeMult   = 1.f; ///< https://www.ssbwiki.com/Hitlag
    float freezeDiMult = 1.f; ///< https://www.ssbwiki.com/Smash_directional_influence

    float knockAngle = 0.f; ///< Knockback angle in degrees (0 = forward, 90 = up).
    float knockBase  = 0.f; ///< Base knockback to apply on collision.
    float knockScale = 0.f; ///< Scale the knockback based on current fighter damage.

    int8_t bone = -1; ///< Index of the bone to attach to.

    uint8_t index = 0u; ///< Used to choose from blobs from the same group.

    BlobAngleMode  angleMode  = {}; ///< https://www.ssbwiki.com/Angle#Special_angles
    BlobFacingMode facingMode = {}; ///< https://www.ssbwiki.com/Angle#Angle_flipper
    BlobClangMode  clangMode  = {}; ///< https://www.ssbwiki.com/Priority
    BlobFlavour    flavour    = {}; ///< Flavour of blob from sour (worst) to sweet (best).

    bool ignoreDamage = false; ///< https://www.ssbwiki.com/Knockback#Set_knockback
    bool ignoreWeight = false; ///< https://www.ssbwiki.com/Knockback#Weight-independent_knockback

    bool canHitGround = true; ///< Can the blob hit targets on the ground.
    bool canHitAir    = true; ///< Can the blob hit targets in the air.

    TinyString  handler = {}; ///< Handler to run on collision.
    SmallString sound   = {}; ///< Sound to play on collision.

    //--------------------------------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    void from_json(const JsonValue& json, const sq::Armature& armature);

    void to_json(JsonValue& json, const sq::Armature& armature) const;

    bool operator==(const HitBlobDef& other) const;
};

//============================================================================//

struct HitBlob final
{
    HitBlob(const HitBlobDef& def, Entity* entity) : def(def), entity(entity) {}

    const HitBlobDef& def;

    Entity* const entity;

    maths::Capsule capsule = {}; ///< Blob capsule after transform.

    bool justCreated = true; ///< Should the blob interpolate.

    bool cancelled = false; ///< Was the blob cancelled by another blob.

    Vec3F get_debug_colour() const;
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobAngleMode, Normal, Sakurai, AutoLink)
SQEE_ENUM_HELPER(sts::BlobFacingMode, Relative, Forward, Reverse)
SQEE_ENUM_HELPER(sts::BlobClangMode, Ignore, Ground, Air, Cancel)
SQEE_ENUM_HELPER(sts::BlobFlavour, Sour, Tangy, Sweet)
