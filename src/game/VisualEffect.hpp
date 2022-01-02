#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include "render/DrawItem.hpp"

#include <sqee/objects/Armature.hpp>

namespace sts {

//============================================================================//

/// Cached resource that is shared between VisualEffects.
struct EffectAsset final
{
    void load_from_directory(const String& path, ResourceCaches& caches);

    sq::Armature armature;
    sq::Armature::Animation animation;

    const sq::Armature::Animation::Track* paramTracks[MAX_EFFECT_BONES] {};

    std::vector<DrawItemDef> drawItemDefs;
};

//============================================================================//

struct VisualEffect final
{
    /// Must be set before calling from_json.
    EffectCache* cache = nullptr;

    /// Fighter that owns this effect.
    Fighter* fighter = nullptr;

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
    bool anchored = true;

    //--------------------------------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    void from_json(const JsonValue& json);

    void to_json(JsonValue& json) const;

    bool operator==(const VisualEffect& other) const;
};

//============================================================================//

} // namespace sts
