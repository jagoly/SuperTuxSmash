#include "game/HitBlob.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void HitBlobDef::from_json(const JsonValue& json, const sq::Armature& armature)
{
    json.at("origin").get_to(origin);
    json.at("radius").get_to(radius);

    json.at("damage").get_to(damage);
    json.at("freezeMult").get_to(freezeMult);
    json.at("freezeDiMult").get_to(freezeDiMult);

    json.at("knockAngle").get_to(knockAngle);
    json.at("knockBase").get_to(knockBase);
    json.at("knockScale").get_to(knockScale);

    bone = armature.bone_from_json(json.at("bone"));

    json.at("index").get_to(index);

    json.at("angleMode").get_to(angleMode);
    json.at("facingMode").get_to(facingMode);
    json.at("clangMode").get_to(clangMode);
    json.at("flavour").get_to(flavour);

    json.at("ignoreDamage").get_to(ignoreDamage);
    json.at("ignoreWeight").get_to(ignoreWeight);

    json.at("canHitGround").get_to(canHitGround);
    json.at("canHitAir").get_to(canHitAir);

    json.at("handler").get_to(handler);
    json.at("sound").get_to(sound);
}

//============================================================================//

void HitBlobDef::to_json(JsonValue& json, const sq::Armature& armature) const
{
    json["origin"] = origin;
    json["radius"] = radius;

    json["damage"] = damage;
    json["freezeMult"] = freezeMult;
    json["freezeDiMult"] = freezeDiMult;

    json["knockAngle"] = knockAngle;
    json["knockBase"] = knockBase;
    json["knockScale"] = knockScale;

    json["bone"] = armature.bone_to_json(bone);

    json["index"] = index;

    json["angleMode"] = angleMode;
    json["facingMode"] = facingMode;
    json["clangMode"] = clangMode;
    json["flavour"] = flavour;

    json["ignoreDamage"] = ignoreDamage;
    json["ignoreWeight"] = ignoreWeight;

    json["canHitGround"] = canHitGround;
    json["canHitAir"] = canHitAir;

    json["handler"] = handler;
    json["sound"] = sound;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool HitBlobDef::operator==(const HitBlobDef& other) const
{
    return origin == other.origin &&
           radius == other.radius &&
           damage == other.damage &&
           freezeMult == other.freezeMult &&
           freezeDiMult == other.freezeDiMult &&
           knockAngle == other.knockAngle &&
           knockBase == other.knockBase &&
           knockScale == other.knockScale &&
           bone == other.bone &&
           index == other.index &&
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
    if (def.flavour == BlobFlavour::Sour)  return { 0.6f, 0.6f, 0.0f };
    if (def.flavour == BlobFlavour::Tangy) return { 0.2f, 1.0f, 0.0f };
    if (def.flavour == BlobFlavour::Sweet) return { 1.0f, 0.1f, 0.1f };
    SQEE_UNREACHABLE();
}
