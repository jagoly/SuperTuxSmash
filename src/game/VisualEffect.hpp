#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include "render/AnimPlayer.hpp"

#include <sqee/objects/Armature.hpp>
#include <sqee/objects/DrawItem.hpp>

namespace sts {

//============================================================================//

/// Cached resource that is shared between effect defs.
struct EffectAsset final
{
    void load_from_directory(const String& path, ResourceCaches& caches);

    sq::Armature armature;
    sq::Animation animation;

    std::vector<sq::DrawItem> drawItems;
};

//============================================================================//

struct VisualEffectDef final
{
    /// Handle to the loaded EffectAsset resource.
    EffectHandle handle = nullptr;

    /// Computed origin/rotation/scale matrix.
    Mat4F localMatrix = {};

    //--------------------------------------------------------//

    /// Path to the EffectAsset to use.
    String path = {};

    Vec3F origin = { 0.f, 0.f, 0.f };
    QuatF rotation = { 0.f, 0.f, 0.f, 1.f };
    Vec3F scale = { 1.f, 1.f, 1.f };

    /// Index of the bone to attach to.
    int8_t bone = -1;

    /// Move the effect with the fighter.
    bool attached = true;

    /// Automatically cancel when the action does.
    bool transient = true;

    //--------------------------------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    void from_json(const JsonValue& json, const sq::Armature& armature, EffectCache& cache);

    void to_json(JsonValue& json, const sq::Armature& armature) const;

    bool operator==(const VisualEffectDef& other) const;
};

//============================================================================//

struct VisualEffect final
{
    VisualEffect(const VisualEffectDef& def, const Entity* entity)
        : def(def), entity(entity), animPlayer(def.handle->armature) {}

    const VisualEffectDef& def;

    const Entity* const entity; // optional

    AnimPlayer animPlayer;

    Mat4F modelMatrix = Mat4F();
    float bbScaleX = 1.f;

    int32_t id = -1;
};

//============================================================================//

} // namespace sts
