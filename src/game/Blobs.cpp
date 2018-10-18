#include "game/Blobs.hpp"

#include <sqee/debug/Logging.hpp>

#include "game/Fighter.hpp"

using namespace sts;

void HitBlob::from_json(const JsonValue& json)
{
    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) sq::log_warning("Invalid bone name %s", jb);
    }
    else bone = -1;

    json.at("origin").get_to(origin);
    json.at("radius").get_to(radius);
    json.at("group").get_to(group);
    json.at("knockAngle").get_to(knockAngle);
    json.at("knockBase").get_to(knockBase);
    json.at("knockScale").get_to(knockScale);
    json.at("damage").get_to(damage);
    json.at("flavour").get_to(flavour);
    json.at("priority").get_to(priority);
}

void HitBlob::to_json(JsonValue& json) const
{
    if (bone != -1)
    {
        const auto boneName = fighter->get_armature().get_bone_name(bone);
        if (boneName.empty()) sq::log_warning("Invalid bone index %d", bone);
        json["bone"] = boneName;
    }
    else json["bone"] = nullptr;

    json["origin"] = origin;
    json["radius"] = radius;
    json["group"] = group;
    json["knockAngle"] = knockAngle;
    json["knockBase"] = knockBase;
    json["knockScale"] = knockScale;
    json["damage"] = damage;
    json["flavour"] = flavour;
    json["priority"] = priority;
}

void HurtBlob::from_json(const JsonValue& item)
{
    bone = int8_t(item[0]);
    originA = Vec3F(item[1], item[2], item[3]);
    originB = Vec3F(item[4], item[5], item[6]);
    radius = float(item[7]);
}

void HurtBlob::to_json(JsonValue& json) const
{
    if (bone != -1)
    {
        const auto boneName = fighter->get_armature().get_bone_name(bone);
        if (boneName.empty()) sq::log_warning("Invalid bone index %d", bone);
        json["bone"] = boneName;
    }
    else json["bone"] = nullptr;

    json["originA"] = originA;
    json["originB"] = originB;
    json["radius"] = radius;
}

SQEE_ENUM_JSON_CONVERSION_DEFINITIONS(sts, HitBlob::Flavour, Sour, Tangy, Sweet)
SQEE_ENUM_JSON_CONVERSION_DEFINITIONS(sts, HitBlob::Priority, Low, Normal, High, Transcend)
