#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include <sqee/vk/Swapper.hpp>
#include <sqee/vk/Vulkan.hpp>

namespace sts {

//============================================================================//

/// Parsed from json, then used to create DrawItems.
struct DrawItemDef final
{
    /// Load draw item definitions from a Render.json file.
    static std::vector<DrawItemDef> load_from_json(const String& path, ResourceCaches& caches);

    TinyString condition;

    MaterialHandle material;
    MeshHandle mesh;

    bool invertCondition = false;
    int8_t subMesh = -1;
};

//============================================================================//

/// Represents a single draw call, objects may have multiple of these.
struct DrawItem final
{
    const bool* condition = nullptr;

    MaterialHandle material;
    MeshHandle mesh;

    bool invertCondition;
    int8_t subMesh;

    // todo: change to vk::DescriptorSet and allow updating using groupId
    // this way there is no worry about reallocated vectors
    const sq::Swapper<vk::DescriptorSet>* descriptorSet;
    int64_t groupId;
};

//============================================================================//

} // namespace sts
