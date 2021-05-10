#pragma once

#include "setup.hpp"

#include <sqee/misc/ResourceCache.hpp>
#include <sqee/misc/ResourceHandle.hpp>
#include <sqee/vk/PassConfig.hpp>

//============================================================================//

namespace sq {

class Mesh;
class Texture;
class Pipeline;
class Material;
class Sound;

class AudioContext;

} // namespace sq

//============================================================================//

namespace sts {

using namespace sq::coretypes;

using MeshCache = sq::ResourceCache<String, sq::Mesh>;
using TextureCache = sq::ResourceCache<String, sq::Texture>;
using PipelineCache = sq::ResourceCache<JsonValue, sq::Pipeline>;
using MaterialCache = sq::ResourceCache<JsonValue, sq::Material>;
using SoundCache = sq::ResourceCache<String, sq::Sound>;

using MeshHandle = sq::Handle<String, sq::Mesh>;
using TextureHandle = sq::Handle<String, sq::Texture>;
using PipelineHandle = sq::Handle<JsonValue, sq::Pipeline>;
using MaterialHandle = sq::Handle<JsonValue, sq::Material>;
using SoundHandle = sq::Handle<String, sq::Sound>;

struct EffectAsset;
using EffectCache = sq::ResourceCache<String, EffectAsset>;
using EffectHandle = sq::Handle<String, EffectAsset>;

//============================================================================//

class ResourceCaches final : sq::NonCopyable
{
public: //================================================//

    ResourceCaches(sq::AudioContext& audio);

    ~ResourceCaches();

    MeshCache meshes;
    TextureCache textures;
    PipelineCache pipelines;
    MaterialCache materials;
    SoundCache sounds;
    EffectCache effects;

    sq::PassConfigMap passConfigMap;

    void refresh_options();

private: //===============================================//

    sq::AudioContext& mAudioContext;
};

//============================================================================//

} // namespace sts
