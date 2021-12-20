#include "render/DrawItem.hpp"

#include <sqee/misc/Json.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>

using namespace sts;

//============================================================================//

std::vector<DrawItemDef> DrawItemDef::load_from_json(const String& path, ResourceCaches& caches)
{
    JsonValue json = sq::parse_json_from_file(path);

    //-- replace pipeline/texture names with assets ----------//

    JsonValue& jsonAssets = json.at("assets");
    JsonValue& jsonMaterials = jsonAssets.at("materials");

    const JsonValue& jsonTextures = jsonAssets.at("textures");
    const JsonValue& jsonPipelines = jsonAssets.at("pipelines");
    const JsonValue& jsonMeshes = jsonAssets.at("meshes");

    for (JsonValue& material : jsonMaterials)
    {
        if (material.is_object() == true)
        {
            JsonValue& pipeline = material.at("pipeline");
            pipeline = jsonPipelines.at(pipeline.get_ref<const String&>());

            for (JsonValue& texture : material.at("textures"))
                texture = jsonTextures.at(texture.get_ref<const String&>());
        }
        else for (JsonValue& materialPass : material)
        {
            JsonValue& pipeline = materialPass.at("pipeline");
            pipeline = jsonPipelines.at(pipeline.get_ref<const String&>());

            for (JsonValue& texture : materialPass.at("textures"))
                texture = jsonTextures.at(texture.get_ref<const String&>());
        }
    }

    //-- populate the result vector --------------------------//

    const JsonValue& jsonItems = json.at("items");

    std::vector<DrawItemDef> result;

    for (const auto& jobj : jsonItems)
    {
        DrawItemDef& def = result.emplace_back();

        if (const auto& condition = jobj.at("condition"); !condition.is_null())
        {
            StringView sv = condition.get_ref<const String&>();
            def.invertCondition = (!sv.empty() && sv.front() == '!');
            if (def.invertCondition) sv.remove_prefix(1u);
            def.condition = sv;
        }

        const String& materialName = jobj.at("material").get_ref<const String&>();
        const JsonValue& materialKey = jsonMaterials.at(materialName);
        def.material = caches.materials.acquire(materialKey);

        const String& meshName = jobj.at("mesh").get_ref<const String&>();
        const String& meshKey = jsonMeshes.at(meshName).get_ref<const String&>();
        def.mesh =  caches.meshes.acquire(meshKey);

        if (const auto& submesh = jobj.at("submesh"); !submesh.is_null())
            submesh.get_to(def.subMesh);
    }

    //--------------------------------------------------------//

    return result;
}
