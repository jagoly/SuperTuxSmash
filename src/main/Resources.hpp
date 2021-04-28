#pragma once

#include "setup.hpp"

#include <sqee/vk/PassConfig.hpp>

#include <sqee/misc/ResourceCache.hpp>
#include <sqee/misc/ResourceHandle.hpp>

//============================================================================//

namespace sq {

class VulkMesh;
class VulkTexture;
//class TextureArray;
class Pipeline;
class VulkMaterial;
class Sound;

class AudioContext;

} // namespace sq

//============================================================================//

namespace sts {

using namespace sq::coretypes;

using MeshCache = sq::ResourceCache<String, sq::VulkMesh>;
using TextureCache = sq::ResourceCache<String, sq::VulkTexture>;
//using TexArrayCache = sq::ResourceCache<String, sq::TextureArray>;
using PipelineCache = sq::ResourceCache<JsonValue, sq::Pipeline>;
using MaterialCache = sq::ResourceCache<JsonValue, sq::VulkMaterial>;
using SoundCache = sq::ResourceCache<String, sq::Sound>;

using MeshHandle = sq::Handle<String, sq::VulkMesh>;
using TextureHandle = sq::Handle<String, sq::VulkTexture>;
//using TexArrayHandle = sq::Handle<String, sq::TextureArray>;
using PipelineHandle = sq::Handle<JsonValue, sq::Pipeline>;
using MaterialHandle = sq::Handle<JsonValue, sq::VulkMaterial>;
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
    //TexArrayCache texarrays;
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
