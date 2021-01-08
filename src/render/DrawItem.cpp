#include "render/DrawItem.hpp"

#include <sqee/misc/Json.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>

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
    const JsonValue& jsonPrograms = jsonAssets.at("programs");
    const JsonValue& jsonMeshes = jsonAssets.at("meshes");

    for (JsonValue& material : jsonMaterials)
    {
        JsonValue& program = material.at("program");
        program = jsonPrograms.at(program.get_ref<const String&>());

        for (JsonValue& uniform : material.at("uniforms"))
            if (uniform.is_string() == true)
                uniform = jsonTextures.at(uniform.get_ref<const String&>());
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
            def.invertCondition = (sv.front() == '!');
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
