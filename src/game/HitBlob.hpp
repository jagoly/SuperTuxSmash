#pragma once

#include <sqee/maths/Volumes.hpp>

#include "game/forward.hpp"

//============================================================================//

namespace sts {

struct HitBlob final : sq::NonCopyable
{
    enum class Type : char
    {
        Offensive, Damageable
    };

    enum class Flavour : char
    {
        Sour, Tangy, Sweet
    };

    enum class Priority : char
    {
        Low, Normal, High, Transcendent
    };

    //--------------------------------------------------------//

    HitBlob(Type type, uint8_t fighter, Action* action)
        : type(type), fighter(fighter), action(action) {}

    //--------------------------------------------------------//

    const Type type;
    const uint8_t fighter;
    Action* const action;

    sq::maths::Sphere sphere;

    //--------------------------------------------------------//

    struct OffensiveProps
    {
        Flavour flavour;
        Priority priority;
        uint8_t group;
        Vec2F direction;
        float damage;
    };

    struct DamageableProps
    {
    };

    //--------------------------------------------------------//

    union
    {
        OffensiveProps offensive;
        DamageableProps damageable;

        char _union_max_size[32];
    };

    //--------------------------------------------------------//

    constexpr Vec3F get_debug_colour()
    {
        switch (type) {
            case Type::Offensive:
            {
                switch (offensive.flavour) {
                    case Flavour::Sour:  return Vec3F(1.0f, 1.0f, 0.0f);
                    case Flavour::Tangy: return Vec3F(1.0f, 0.6f, 0.2f);
                    case Flavour::Sweet: return Vec3F(1.0f, 0.6f, 0.6f);
                } // switch (offensive.flavour)
            }
            case Type::Damageable: return Vec3F(0.5, 0.5, 1.0);
        } // switch (type)
    }
};

static_assert(sizeof(HitBlob) == 64u);
static_assert(std::is_trivially_destructible_v<HitBlob>);

} // namespace sts
