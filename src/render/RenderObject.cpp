#include "render/RenderObject.hpp"

#include "render/Renderer.hpp"

#include <sqee/gl/Context.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>

using namespace sts;

//============================================================================//

void RenderObject::base_load_from_json(const String& path, std::map<TinyString, const bool*> conditions)
{
    JsonValue json = sq::parse_json_from_file(path);

    // non-const because we need to edit the material entries
    JsonValue& assets = json.at("assets");
    JsonValue& materials = assets.at("materials");

    const JsonValue& textures = assets.at("textures");
    const JsonValue& programs = assets.at("programs");

    const JsonValue& meshes = assets.at("meshes");

    // json values are used as resource keys, so replace program/texture names with actual data
    for (JsonValue& material : materials)
    {
        JsonValue& program = material.at("program");
        program = programs.at(program.get_ref<const String&>());

        for (JsonValue& uniform : material.at("uniforms"))
            if (uniform.is_string() == true)
                uniform = textures.at(uniform.get_ref<const String&>());
    }

    // create opaque renderables from the json
    for (const auto& jobj : json.at("pass_opaque"))
    {
        OpaqueObject& obj = mOpaqueObjects.emplace_back();

        // if renderable has a condition, get it from from the map passed to this function
        if (auto& condition = jobj.at("condition"); !condition.is_null())
        {
            StringView sv = condition.get_ref<const String&>();
            if (sv.front() == '!')
            {
                obj.invertCondition = true;
                sv.remove_prefix(1u);
            }
            obj.condition = conditions.at(sv);
        }

        const String& materialName = jobj.at("material");
        const JsonValue& materialKey = materials.at(materialName);
        obj.material = renderer.materials.acquire(materialKey);

        const String& meshName = jobj.at("mesh");
        const String& meshKey = meshes.at(meshName);
        obj.mesh = renderer.meshes.acquire(meshKey);

        // todo: might be nice give submeshes names rather than just indices
        if (auto& submesh = jobj.at("submesh"); !submesh.is_null())
            obj.subMesh = submesh;
    }
}

//============================================================================//

void RenderObject::base_render_opaque() const
{
    auto& context = renderer.context;

    context.set_state(sq::DepthTest::Replace);
    context.set_depth_compare(sq::CompareFunc::LessEqual);

    for (const OpaqueObject& obj : mOpaqueObjects)
    {
        if (obj.condition && *obj.condition == obj.invertCondition)
            continue;

        obj.material->apply_to_context(context);
        obj.mesh->apply_to_context(context);

        if (obj.subMesh < 0) obj.mesh->draw_complete(context);
        else obj.mesh->draw_submesh(context, uint(obj.subMesh));
    }
}
