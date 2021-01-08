#pragma once

#include "setup.hpp" // IWYU pragma: export

#include "main/Resources.hpp"

namespace sq { class FixedBuffer; }

namespace sts {

//============================================================================//

/// Defines when a DrawItem should be drawn.
enum class DrawPass : int8_t { Opaque, Transparent };

//============================================================================//

/// Parsed from json, then used to create DrawItems.
struct DrawItemDef final
{
    /// Load draw item definitions from a Render.json file.
    static std::vector<DrawItemDef> load_from_json(const String& path, ResourceCaches& caches);

    TinyString condition;

    MaterialHandle material;
    MeshHandle mesh;

    DrawPass pass;
    bool invertCondition;
    int8_t subMesh;
};

//============================================================================//

/// Represents a single draw call, objects may have multiple of these.
struct DrawItem final
{
    const bool* condition;

    MaterialHandle material;
    MeshHandle mesh;

    DrawPass pass;
    bool invertCondition;
    int8_t subMesh;

    const sq::FixedBuffer* ubo;
    int64_t groupId;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::DrawPass, Opaque, Transparent)
