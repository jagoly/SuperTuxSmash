#include "render/DrawItem.hpp"

#include <sqee/misc/Json.hpp>
#include <sqee/vk/VulkMaterial.hpp>
#include <sqee/vk/VulkMesh.hpp>

using namespace sts;

//============================================================================//

SQEE_ENUM_JSON_CONVERSIONS(sts::DrawPass)

//============================================================================//

std::vector<DrawItemDef> DrawItemDef::load_from_json(const String& path, ResourceCaches& caches)
{
    JsonValue json = sq::parse_json_from_file(path);

    //-- replace program/texture names with assets -----------//

    JsonValue& jsonAssets = json.at("assets");
    JsonValue& jsonMaterials = jsonAssets.at("materials");

    const JsonValue& jsonTextures = jsonAssets.at("textures");
    const JsonValue& jsonPipelines = jsonAssets.at("pipelines");
    const JsonValue& jsonMeshes = jsonAssets.at("meshes");

    for (JsonValue& material : jsonMaterials)
    {
        JsonValue& pipeline = material.at("pipeline");
        pipeline = jsonPipelines.at(pipeline.get_ref<const String&>());

        for (JsonValue& texture : material.at("textures"))
            texture = jsonTextures.at(texture.get_ref<const String&>());
    }

    //-- populate the result vector --------------------------//

    const JsonValue& jsonItems = json.at("items");

    std::vector<DrawItemDef> result;
    result.reserve(jsonItems.size());

    for (const auto& jobj : jsonItems)
    {
        DrawItemDef& def = result.emplace_back();

        if (auto& condition = jobj.at("condition"); !condition.is_null())
        {
            StringView sv = condition.get_ref<const String&>();
            def.invertCondition = (!sv.empty() && sv.front() == '!');
            if (def.invertCondition) sv.remove_prefix(1u);
            def.condition = sv;
        }

        const String& materialName = jobj.at("material");
        const JsonValue& materialKey = jsonMaterials.at(materialName);
        def.material = caches.materials.acquire(materialKey);

        const String& meshName = jobj.at("mesh");
        const String& meshKey = jsonMeshes.at(meshName);
        def.mesh =  caches.meshes.acquire(meshKey);

        def.pass = jobj.at("pass");

        if (auto& submesh = jobj.at("submesh"); !submesh.is_null())
            def.subMesh = submesh;
    }

    //--------------------------------------------------------//

    // todo: sort the result before returning it

    return result;
}
