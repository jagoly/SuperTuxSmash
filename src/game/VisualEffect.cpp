#include "game/VisualEffect.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void EffectAsset::load_from_directory(const String& path, ResourceCaches& caches)
{
    armature.load_from_file(path + "/Armature.json");
    animation = armature.make_animation(path + "/Animation.sqa");

    SQASSERT(armature.get_bone_count() <= MAX_EFFECT_BONES, "too many bones for visual effect");

    // todo: rename track to be more generic. probably "params"
    for (uint i = 0u; i < animation.boneCount; ++i)
        paramTracks[i] = animation.find_extra(i, "colour");

    drawItemDefs = DrawItemDef::load_from_json(path + "/Render.json", caches);
}

//============================================================================//

void VisualEffect::from_json(const JsonValue& json)
{
    json.at("path").get_to(path);

    json.at("origin").get_to(origin);
    json.at("rotation").get_to(rotation);
    json.at("scale").get_to(scale);

    json.at("anchored").get_to(anchored);

    handle = cache->acquire(path);

    localMatrix = maths::transform(origin, rotation, scale);
}

//============================================================================//

void VisualEffect::to_json(JsonValue& json) const
{
    json["path"] = path;

    json["origin"] = origin;
    json["rotation"] = rotation;
    json["scale"] = scale;

    json["anchored"] = anchored;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool sts::operator==(const VisualEffect& a, const VisualEffect& b)
{
    if (a.path != b.path) return false;
    if (a.origin != b.origin) return false;
    if (a.rotation != b.rotation) return false;
    if (a.scale != b.scale) return false;
    if (a.anchored != b.anchored) return false;

    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY()
