#include "game/HurtBlob.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

SQEE_ENUM_JSON_CONVERSIONS(sts::BlobRegion)

//============================================================================//

void HurtBlob::from_json(const JsonValue& json)
{
    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) throw std::invalid_argument("invalid bone '{}'"_format(jb));
    }
    else bone = -1;

    json.at("originA").get_to(originA);
    json.at("originB").get_to(originB);
    json.at("radius").get_to(radius);

    json.at("region").get_to(region);
}

//============================================================================//

void HurtBlob::to_json(JsonValue& json) const
{
    if (bone == -1) json["bone"] = nullptr;
    else json["bone"] = fighter->get_armature().get_bone_name(bone);

    json["originA"] = originA;
    json["originB"] = originB;
    json["radius"] = radius;

    json["region"] = region;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool sts::operator==(const HurtBlob& a, const HurtBlob& b)
{
    if (a.originA != b.originA) return false;
    if (a.originB != b.originB) return false;
    if (a.radius != b.radius) return false;
    if (a.bone != b.bone) return false;
    if (a.region != b.region) return false;

    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY()
