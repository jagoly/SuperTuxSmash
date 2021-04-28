#include "main/Resources.hpp"

#include "game/VisualEffect.hpp"

#include <sqee/app/AudioContext.hpp>

#include <sqee/vk/VulkMesh.hpp>
#include <sqee/vk/VulkTexture.hpp>
#include <sqee/vk/Pipeline.hpp>
#include <sqee/vk/VulkMaterial.hpp>
#include <sqee/objects/Sound.hpp>

#include <sqee/misc/Json.hpp>
#include <sqee/misc/Files.hpp>

using namespace sts;

//============================================================================//

ResourceCaches::ResourceCaches(sq::AudioContext& audio)
    : mAudioContext(audio)
{
    //-- Assign Factory Functions ----------------------------//

    meshes.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::VulkMesh>();
        result->load_from_file(sq::build_string("assets/", key, ".sqm"), true);
        return result;
    });

    textures.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::VulkTexture>();
        result->load_from_file_2D(sq::build_string("assets/", key));
        return result;
    });

//    texarrays.assign_factory([](const String& key)
//    {
//        auto result = std::make_unique<sq::TextureArray>();
//        result->load_automatic(sq::build_string("assets/", key));
//        return result;
//    });

    pipelines.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::Pipeline>();
        result->load_from_json(key, passConfigMap);
        return result;
    });

    materials.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::VulkMaterial>();
        result->load_from_json(key, pipelines, textures);
        return result;
    });

    sounds.assign_factory([this](const String& key)
    {
        auto path = sq::compute_resource_path(key, {"assets/"}, {".wav", ".flac"});

        auto result = std::make_unique<sq::Sound>(mAudioContext);
        result->load_from_file(path);
        return result;
    });

    effects.assign_factory([this](const String& key)
    {
        auto result = std::make_unique<EffectAsset>();
        result->load_from_directory(sq::build_string("assets/", key), *this);
        return result;
    });
}

ResourceCaches::~ResourceCaches() = default;

//============================================================================//

void ResourceCaches::refresh_options()
{
    pipelines.reload_resources();
}
