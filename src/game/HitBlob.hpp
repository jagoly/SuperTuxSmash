#pragma once

#include <sqee/maths/Volumes.hpp>

#include "game/forward.hpp"

//============================================================================//

namespace sts {

struct HitBlob final : sq::NonCopyable
{
    enum class Type : int8_t { Offensive, Damageable };
    enum class Flavour : int8_t { Sour, Tangy, Sweet };

    //--------------------------------------------------------//

    HitBlob(Type type, Fighter* fighter, Action* action)
        : type(type), fighter(fighter), action(action) {}

    //--------------------------------------------------------//

    const Type type;

    Fighter* const fighter;
    Action* const action;

    sq::maths::Sphere sphere;

    //--------------------------------------------------------//

    struct OffensiveProps
    {
        Flavour flavour;
    };

    struct DamageableProps
    {
    };

    //--------------------------------------------------------//

    union
    {
        OffensiveProps offensive;
        DamageableProps damageable;
        char _union_max_size[24];
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
