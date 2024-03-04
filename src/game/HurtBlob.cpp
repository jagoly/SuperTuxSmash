#include "game/HurtBlob.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void HurtBlobDef::from_json(JsonObject json, const sq::Armature& armature)
{
    originA = json["originA"].as_auto();
    originB = json["originB"].as_auto();
    radius = json["radius"].as_auto();

    bone = armature.json_as_bone_index(json["bone"]);

    region = json["region"].as_auto();
}

//============================================================================//

void HurtBlobDef::to_json(JsonMutObject json, const sq::Armature& armature) const
{
    json.append("originA", originA);
    json.append("originB", originB);
    json.append("radius", radius);

    json.append("bone", armature.json_from_bone_index(json.document(), bone));

    json.append("region", region);
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool HurtBlobDef::operator==(const HurtBlobDef& other) const
{
    return originA == other.originA &&
           originB == other.originB &&
           radius == other.radius &&
           bone == other.bone &&
           region == other.region;
}

ENABLE_WARNING_FLOAT_EQUALITY()

//============================================================================//

Vec3F HurtBlob::get_debug_colour() const
{
    if (def.region == BlobRegion::Middle) return { 0.4f, 0.4f, 1.0f };
    if (def.region == BlobRegion::Lower)  return { 0.8f, 0.0f, 1.0f };
    if (def.region == BlobRegion::Upper)  return { 0.0f, 0.8f, 1.0f };
    SQEE_UNREACHABLE();
}
