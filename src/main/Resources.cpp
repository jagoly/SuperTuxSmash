#include "main/Resources.hpp"

#include "game/VisualEffect.hpp"

#include <sqee/app/AudioContext.hpp>

#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>
#include <sqee/objects/Pipeline.hpp>
#include <sqee/objects/Sound.hpp>
#include <sqee/objects/Texture.hpp>

#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

ResourceCaches::ResourceCaches(sq::AudioContext& audio)
    : mAudioContext(audio)
{
    //-- Assign Factory Functions ----------------------------//

    meshes.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::Mesh>();
        result->load_from_file(sq::build_string("assets/", key, ".sqm"), true);
        return result;
    });

    textures.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::Texture>();
        result->load_from_file_2D(sq::build_string("assets/", key));
        return result;
    });

    pipelines.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::Pipeline>();
        result->load_from_json(key, passConfigMap);
        return result;
    });

    materials.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::Material>();
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
    effects.free_unreachable();
    sounds.free_unreachable();
    materials.free_unreachable();
    pipelines.free_unreachable();
    textures.free_unreachable();
    meshes.free_unreachable();

    pipelines.reload_resources();
}
