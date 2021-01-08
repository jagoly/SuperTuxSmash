#include "main/Resources.hpp"

#include "game/VisualEffect.hpp"

#include <sqee/app/AudioContext.hpp>
#include <sqee/app/PreProcessor.hpp>

#include <sqee/objects/Mesh.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Sound.hpp>

#include <sqee/misc/Json.hpp>
#include <sqee/misc/Files.hpp>

using namespace sts;

//============================================================================//

ResourceCaches::ResourceCaches(sq::AudioContext& audio, sq::PreProcessor& processor)
    : mAudioContext(audio), mPreProcessor(processor)
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
        auto result = std::make_unique<sq::Texture2D>();
        result->load_automatic(sq::build_string("assets/", key));
        return result;
    });

    texarrays.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::TextureArray>();
        result->load_automatic(sq::build_string("assets/", key));
        return result;
    });

    programs.assign_factory([this](const JsonValue& key)
    {
        const String& path = key.at("path");
        sq::PreProcessor::OptionMap options = key.at("options");

        auto result = std::make_unique<sq::Program>();
        mPreProcessor.load_super_shader(*result, sq::build_string("shaders/", path, ".glsl"), options);
        return result;
    });

    materials.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::Material>();
        result->load_from_json(key, programs, textures);
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
