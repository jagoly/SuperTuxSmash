#include "game/HitBlob.hpp"

#include "game/Action.hpp"
#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

SQEE_ENUM_JSON_CONVERSIONS(sts::BlobFacing)
SQEE_ENUM_JSON_CONVERSIONS(sts::BlobFlavour)

//============================================================================//

void HitBlob::from_json(const JsonValue& json)
{
    SQASSERT(action != nullptr, "");

    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = action->fighter.get_armature().get_bone_index(jb);
        if (bone == -1) throw std::invalid_argument("invalid bone '{}'"_format(jb));
    }
    else bone = -1;

    json.at("origin").get_to(origin);
    json.at("radius").get_to(radius);

    json.at("group").get_to(group);
    json.at("index").get_to(index);

    json.at("damage").get_to(damage);
    json.at("freezeFactor").get_to(freezeFactor);
    json.at("knockAngle").get_to(knockAngle);
    json.at("knockBase").get_to(knockBase);
    json.at("knockScale").get_to(knockScale);

    json.at("facing").get_to(facing);
    json.at("flavour").get_to(flavour);

    json.at("useFixedKnockback").get_to(useFixedKnockback);
    json.at("useSakuraiAngle").get_to(useSakuraiAngle);
}

//============================================================================//

void HitBlob::to_json(JsonValue& json) const
{
    SQASSERT(action != nullptr, "");

    if (bone == -1) json["bone"] = nullptr;
    else json["bone"] = action->fighter.get_armature().get_bone_name(bone);

    json["origin"] = origin;
    json["radius"] = radius;

    json["group"] = group;
    json["index"] = index;

    json["damage"] = damage;
    json["freezeFactor"] = freezeFactor;
    json["knockAngle"] = knockAngle;
    json["knockBase"] = knockBase;
    json["knockScale"] = knockScale;

    json["facing"] = facing;
    json["flavour"] = flavour;

    json["useFixedKnockback"] = useFixedKnockback;
    json["useSakuraiAngle"] = useSakuraiAngle;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY;

bool sts::operator==(const HitBlob& a, const HitBlob& b)
{
    if (a.origin != b.origin) return false;
    if (a.radius != b.radius) return false;
    if (a.bone != b.bone) return false;
    if (a.group != b.group) return false;
    if (a.index != b.index) return false;
    if (a.damage != b.damage) return false;
    if (a.freezeFactor != b.freezeFactor) return false;
    if (a.knockAngle != b.knockAngle) return false;
    if (a.knockBase != b.knockBase) return false;
    if (a.knockScale != b.knockScale) return false;
    if (a.facing != b.facing) return false;
    if (a.flavour != b.flavour) return false;
    if (a.useFixedKnockback != b.useFixedKnockback) return false;
    if (a.useSakuraiAngle != b.useSakuraiAngle) return false;

    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY;
