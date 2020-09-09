#pragma once

#include "setup.hpp"

#include <sqee/maths/Volumes.hpp> // IWYU pragma: export

namespace sts {

//============================================================================//

/// the order of this matters as it's used for priority
enum class BlobRegion : int8_t { Middle, Lower, Upper };

//============================================================================//

struct alignas(16) HurtBlob final
{
    Fighter* fighter = nullptr; ///< Fighter that owns this blob.

    maths::Capsule capsule; ///< Blob capsule after transform.

    //--------------------------------------------------------//

    Vec3F originA; ///< Local first origin of the blob capsule.
    Vec3F originB; ///< Local second origin of the blob capsule.
    float radius;  ///< Local radius of the blob capsule.

    int8_t bone; ///< Index of the bone to attach to.

    uint8_t index; ///< Index of the blob within the fighter.

    BlobRegion region; ///< What animations should this trigger.

    //-- debug / editor methods ------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    constexpr Vec3F get_debug_colour() const
    {
        SWITCH (region) {
        CASE (Lower)  return { 0.8f, 0.0f, 1.0f };
        CASE (Middle) return { 0.4f, 0.4f, 1.0f };
        CASE (Upper)  return { 0.0f, 0.8f, 1.0f };
        CASE_DEFAULT  throw std::exception();
        } SWITCH_END;
    }

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

bool operator==(const HurtBlob& a, const HurtBlob& b);

//============================================================================//

} // namespace sts

SQEE_ENUM_HELPER(sts::BlobRegion, Middle, Lower, Upper)
