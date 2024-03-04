#include "game/VisualEffect.hpp"

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

    // todo: change to wren expressions
    // todo: effects should probably evaluate in the context of the entity that spawned them
    for (const sq::DrawItem& drawItem : drawItems)
    {
        if (drawItem.condition.empty()) continue;
        sq::log_warning("'{}/Render.json': invalid condition '{}'", path, drawItem.condition);
    }
}

//============================================================================//

void VisualEffectDef::from_json(JsonObject json, const sq::Armature& armature, EffectCache& cache)
{
    path = json["path"].as_auto();

    origin = json["origin"].as_auto();
    rotation = json["rotation"].as_auto();
    scale = json["scale"].as_auto();

    bone = armature.json_as_bone_index(json["bone"]);

    attached = json["attached"].as_auto();
    transient = json["transient"].as_auto();

    handle = cache.acquire(path);

    localMatrix = maths::transform(origin, rotation, scale);
}

//============================================================================//

void VisualEffectDef::to_json(JsonMutObject json, const sq::Armature& armature) const
{
    json.append("path", path);

    json.append("origin", origin);
    json.append("rotation", rotation);
    json.append("scale", scale);

    json.append("bone", armature.json_from_bone_index(json.document(), bone));

    json.append("attached", attached);
    json.append("transient", transient);
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
