#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

/// order matters here as it's used for priority
enum class BlobRegion : int8_t { Middle, Lower, Upper };

//============================================================================//

struct HurtBlob final
{
    Fighter* fighter = nullptr; ///< Fighter that owns this blob.

    maths::Capsule capsule; ///< Blob capsule after transform.

    //--------------------------------------------------------//

    Vec3F originA; ///< Model space origin of first end of the capsule.
    Vec3F originB; ///< Model space origin of second end of the capsule.
    float radius;  ///< Model space radius of the capsule.

    int8_t bone; ///< Index of the bone to attach to.

    BlobRegion region; ///< What animations should this trigger.

    //--------------------------------------------------------//

    void from_json(const JsonValue& json);

    void to_json(JsonValue& json) const;

    Vec3F get_debug_colour() const;

    bool operator==(const HurtBlob& other) const;

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }
};

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobRegion, Middle, Lower, Upper)
