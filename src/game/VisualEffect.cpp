#include "game/VisualEffect.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void EffectAsset::load_from_directory(const String& path, ResourceCaches& caches)
{
    armature.load_from_file(path + "/Armature.json");
    animation = armature.load_animation_from_file(path + "/Animation");

    drawItems = sq::DrawItem::load_from_json (
        path + "/Render.json", armature,
        caches.meshes, caches.pipelines, caches.textures
    );
}

//============================================================================//

void VisualEffectDef::from_json(const JsonValue& json, const sq::Armature& armature, EffectCache& cache)
{
    json.at("path").get_to(path);

    json.at("origin").get_to(origin);
    json.at("rotation").get_to(rotation);
    json.at("scale").get_to(scale);

    bone = armature.bone_from_json(json.at("bone"));

    json.at("attached").get_to(attached);
    json.at("transient").get_to(transient);

    handle = cache.acquire(path);

    localMatrix = maths::transform(origin, rotation, scale);
}

//============================================================================//

void VisualEffectDef::to_json(JsonValue& json, const sq::Armature& armature) const
{
    json["path"] = path;

    json["origin"] = origin;
    json["rotation"] = rotation;
    json["scale"] = scale;

    json["bone"] = armature.bone_to_json(bone);

    json["attached"] = attached;
    json["transient"] = transient;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool VisualEffectDef::operator==(const VisualEffectDef& other) const
{
    return path == other.path &&
           origin == other.origin &&
           rotation == other.rotation &&
           scale == other.scale &&
           bone == other.bone &&
           attached == other.attached &&
           transient == other.transient;
}

ENABLE_WARNING_FLOAT_EQUALITY()
