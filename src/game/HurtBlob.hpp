#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp>
#include <sqee/objects/Armature.hpp>

namespace sts {

//============================================================================//

/// Kind of animation to play when getting hit.
enum class BlobRegion : int8_t { Middle, Lower, Upper };

//============================================================================//

struct HurtBlobDef final
{
    Vec3F originA =  {}; ///< Model space origin of first end of the capsule.
    Vec3F originB =  {}; ///< Model space origin of second end of the capsule.
    float radius  = 0.f; ///< Model space radius of the capsule.

    int8_t bone = -1; ///< Index of the bone to attach to.

    BlobRegion region = {}; ///< What animations should this trigger.

    //--------------------------------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    void from_json(const JsonValue& json, const sq::Armature& armature);

    void to_json(JsonValue& json, const sq::Armature& armature) const;

    bool operator==(const HurtBlobDef& other) const;
};

//============================================================================//

struct HurtBlob final
{
    HurtBlob(const HurtBlobDef& def, Fighter& fighter) : def(def), fighter(fighter) {}

    const HurtBlobDef& def;

    Fighter& fighter;

    maths::Capsule capsule = {}; ///< Blob capsule after transform.

    bool intangible = false; ///< Is the blob disabled.

    Vec3F get_debug_colour() const;
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobRegion, Middle, Lower, Upper)
