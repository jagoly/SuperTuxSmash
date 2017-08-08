#pragma once

#include <sqee/builtins.hpp>

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

    Fighter* fighter;  ///< The fighter who owns this blob.
    Action* action;    ///< The action that created this blob.

    Vec3F origin;  ///< Local origin of the blob sphere.
    float radius;  ///< Local radius of the blob sphere.

    sq::maths::Sphere sphere;  ///< Blob sphere after transform.

    int8_t bone;  ///< Index of the bone to attach to.

    //--------------------------------------------------------//

    uint8_t group;  ///< Groups may only collide once per fighter per action.

    Flavour flavour;    ///< Flavour of blob from sour (worst) to sweet (best).
    Priority priority;  ///< Priority of blob when colliding with other hit blobs.

    char _paddingA[4];

    float damage;      ///< How much damage the blob will do when hit.
    float knockAngle;  ///< Angle of knockback in turns (0.0 == up, 0.25 == xdir, 0.5 == down)
    float knockBase;   ///< Base knockback to apply on collision.
    float knockScale;  ///< Scale the knockback based on current fighter damage.

    char _paddingB[24];

    //--------------------------------------------------------//

    void set_flavour_from_str(const string& str)
    {
        if      (str == "SOUR")  flavour = Flavour::Sour;
        else if (str == "TANGY") flavour = Flavour::Tangy;
        else if (str == "SWEET") flavour = Flavour::Sweet;
        else throw std::invalid_argument("bad flavour");
    }

    void set_priority_from_str(const string& str)
    {
        if      (str == "LOW")       priority = Priority::Low;
        else if (str == "NORMAL")    priority = Priority::Normal;
        else if (str == "HIGH")      priority = Priority::High;
        else if (str == "TRANSCEND") priority = Priority::Transcend;
        else throw std::invalid_argument("bad priority");
    }

    //--------------------------------------------------------//

    constexpr Vec3F get_debug_colour() const
    {
        if (flavour == Flavour::Sour)  return { 1.0f, 1.0f, 0.0f };
        if (flavour == Flavour::Tangy) return { 1.0f, 1.0f, 0.0f };
        if (flavour == Flavour::Sweet) return { 1.0f, 0.6f, 0.6f };
        return {};
    }
};

//============================================================================//

struct alignas(16) HurtBlob final
{
    HurtBlob(Fighter& fighter) : fighter(&fighter) {}

    //--------------------------------------------------------//

    Fighter* fighter;  ///< The fighter who owns this blob.

    Vec3F originA;  ///< Local first origin of the blob capsule.
    Vec3F originB;  ///< Local second origin of the blob capsule.
    float radius;   ///< Local radius of the blob capsule.

    sq::maths::Capsule capsule;  ///< Blob capsule after transform.

    int8_t bone;  ///< Index of the bone to attach to.

    char _padding[15];

    //--------------------------------------------------------//

    constexpr Vec3F get_debug_colour() const
    {
        return { 0.5f, 0.5f, 1.0f };
    }
};

//============================================================================//

static_assert(sizeof(HitBlob) == 96u);
static_assert(std::is_trivially_destructible_v<HitBlob>);
static_assert(std::is_trivially_copyable_v<HitBlob>);

static_assert(sizeof(HurtBlob) == 80u);
static_assert(std::is_trivially_destructible_v<HurtBlob>);
static_assert(std::is_trivially_copyable_v<HurtBlob>);

//============================================================================//

} // namespace sts
