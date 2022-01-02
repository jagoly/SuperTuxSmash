#include "game/VisualEffect.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void EffectAsset::load_from_directory(const String& path, ResourceCaches& caches)
{
    armature.load_from_file(path + "/Armature.json");
    animation = armature.load_animation_from_file(path + "/Animation");

    if (armature.get_bone_count() > MAX_EFFECT_BONES)
        SQEE_THROW("too many bones for a visual effect");

    // todo: rename track to be more generic. probably "params"
    for (uint i = 0u; i < animation.boneCount; ++i)
        paramTracks[i] = animation.find_extra(i, "colour");

    drawItemDefs = DrawItemDef::load_from_json(path + "/Render.json", caches);
}

//============================================================================//

void VisualEffect::from_json(const JsonValue& json)
{
    SQASSERT(fighter != nullptr, "");

    json.at("path").get_to(path);

    json.at("origin").get_to(origin);
    json.at("rotation").get_to(rotation);
    json.at("scale").get_to(scale);

    bone = fighter->bone_from_json(json.at("bone"));

    json.at("anchored").get_to(anchored);

    handle = cache->acquire(path);

    localMatrix = maths::transform(origin, rotation, scale);
}

//============================================================================//

void VisualEffect::to_json(JsonValue& json) const
{
    SQASSERT(fighter != nullptr, "");

    json["path"] = path;

    json["origin"] = origin;
    json["rotation"] = rotation;
    json["scale"] = scale;

    json["bone"] = fighter->bone_to_json(bone);

    json["anchored"] = anchored;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool VisualEffect::operator==(const VisualEffect& other) const
{
    return path == other.path &&
           origin == other.origin &&
           rotation == other.rotation &&
           scale == other.scale &&
           bone == other.bone &&
           anchored == other.anchored;
}

ENABLE_WARNING_FLOAT_EQUALITY()
