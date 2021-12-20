#include "game/HurtBlob.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void HurtBlob::from_json(const JsonValue& json)
{
    SQASSERT(fighter != nullptr, "");

    json.at("originA").get_to(originA);
    json.at("originB").get_to(originB);
    json.at("radius").get_to(radius);

    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        const auto& key = jb.get_ref<const String&>();
        bone = fighter->get_armature().get_bone_index(key);
        if (bone == -1) SQEE_THROW("invalid bone '{}'", key);
    }
    else bone = -1;

    json.at("region").get_to(region);
}

//============================================================================//

void HurtBlob::to_json(JsonValue& json) const
{
    json["originA"] = originA;
    json["originB"] = originB;
    json["radius"] = radius;

    if (bone == -1) json["bone"] = nullptr;
    else json["bone"] = fighter->get_armature().get_bone_name(bone);

    json["region"] = region;
}

//============================================================================//

Vec3F HurtBlob::get_debug_colour() const
{
    if (region == BlobRegion::Middle) return { 0.4f, 0.4f, 1.0f };
    if (region == BlobRegion::Lower)  return { 0.8f, 0.0f, 1.0f };
    if (region == BlobRegion::Upper)  return { 0.0f, 0.8f, 1.0f };
    SQEE_UNREACHABLE();
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool HurtBlob::operator==(const HurtBlob& other) const
{
    return originA == other.originA &&
           originB == other.originB &&
           radius == other.radius &&
           bone == other.bone &&
           region == other.region;
}

ENABLE_WARNING_FLOAT_EQUALITY()
