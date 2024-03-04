#include "game/HitBlob.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void HitBlobDef::from_json(JsonObject json, const sq::Armature& armature)
{
    origin = json["origin"].as_auto();
    radius = json["radius"].as_auto();

    bone = armature.json_as_bone_index(json["bone"]);

    index = json["index"].as_auto();
    // type = json["type"].as_auto();
    if (auto jType = json.get_safe("type")) type = jType->as_auto();
    else type = BlobType::Damage;

    damage = json["damage"].as_auto();
    freezeMult = json["freezeMult"].as_auto();
    freezeDiMult = json["freezeDiMult"].as_auto();

    knockAngle = json["knockAngle"].as_auto();
    knockBase = json["knockBase"].as_auto();
    knockScale = json["knockScale"].as_auto();

    angleMode = json["angleMode"].as_auto();
    facingMode = json["facingMode"].as_auto();
    clangMode = json["clangMode"].as_auto();
    flavour = json["flavour"].as_auto();

    ignoreDamage = json["ignoreDamage"].as_auto();
    ignoreWeight = json["ignoreWeight"].as_auto();

    canHitGround = json["canHitGround"].as_auto();
    canHitAir = json["canHitAir"].as_auto();

    handler = json["handler"].as_auto();
    sound = json["sound"].as_auto();
}

//============================================================================//

void HitBlobDef::to_json(JsonMutObject json, const sq::Armature& armature) const
{
    json.append("origin", origin);
    json.append("radius", radius);

    json.append("bone", armature.json_from_bone_index(json.document(), bone));

    json.append("index", index);
    json.append("type", type);

    json.append("damage", damage);
    json.append("freezeMult", freezeMult);
    json.append("freezeDiMult", freezeDiMult);

    json.append("knockAngle", knockAngle);
    json.append("knockBase", knockBase);
    json.append("knockScale", knockScale);

    json.append("angleMode", angleMode);
    json.append("facingMode", facingMode);
    json.append("clangMode", clangMode);
    json.append("flavour", flavour);

    json.append("ignoreDamage", ignoreDamage);
    json.append("ignoreWeight", ignoreWeight);

    json.append("canHitGround", canHitGround);
    json.append("canHitAir", canHitAir);

    json.append("handler", handler);
    json.append("sound", sound);
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool HitBlobDef::operator==(const HitBlobDef& other) const
{
    return origin == other.origin &&
           radius == other.radius &&
           bone == other.bone &&
           index == other.index &&
           type == other.type &&
           damage == other.damage &&
           freezeMult == other.freezeMult &&
           freezeDiMult == other.freezeDiMult &&
           knockAngle == other.knockAngle &&
           knockBase == other.knockBase &&
           knockScale == other.knockScale &&
           angleMode == other.angleMode &&
           facingMode == other.facingMode &&
           clangMode == other.clangMode &&
           flavour == other.flavour &&
           ignoreDamage == other.ignoreDamage &&
           ignoreWeight == other.ignoreWeight &&
           canHitGround == other.canHitGround &&
           canHitAir == other.canHitAir &&
           handler == other.handler &&
           sound == other.sound;
}

ENABLE_WARNING_FLOAT_EQUALITY()

//============================================================================//

Vec3F HitBlob::get_debug_colour() const
{
    if (def.type == BlobType::Damage)
    {
        if (def.flavour == BlobFlavour::Sour)  return { 0.2f, 1.0f, 0.0f };
        if (def.flavour == BlobFlavour::Tangy) return { 0.6f, 0.6f, 0.0f };
        if (def.flavour == BlobFlavour::Sweet) return { 1.0f, 0.2f, 0.0f };
    }
    if (def.type == BlobType::Grab) return { 0.8f, 0.8f, 0.8f };

    SQEE_UNREACHABLE();
}
