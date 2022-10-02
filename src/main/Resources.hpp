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
class Sound;

class AudioContext;

} // namespace sq

//============================================================================//

namespace sts {

using namespace sq::coretypes;

using MeshCache = sq::ResourceCache<String, sq::Mesh>;
using TextureCache = sq::ResourceCache<String, sq::Texture>;
using PipelineCache = sq::ResourceCache<JsonValue, sq::Pipeline>;
using SoundCache = sq::ResourceCache<String, sq::Sound>;

using MeshHandle = sq::Handle<String, sq::Mesh>;
using TextureHandle = sq::Handle<String, sq::Texture>;
using PipelineHandle = sq::Handle<JsonValue, sq::Pipeline>;
using SoundHandle = sq::Handle<String, sq::Sound>;

struct EffectAsset;
using EffectCache = sq::ResourceCache<String, EffectAsset>;
using EffectHandle = sq::Handle<String, EffectAsset>;

//============================================================================//

class ResourceCaches final
{
public: //================================================//

    ResourceCaches(sq::AudioContext& audio);

    SQEE_COPY_DELETE(ResourceCaches)
    SQEE_MOVE_DELETE(ResourceCaches)

    ~ResourceCaches();

    MeshCache meshes;
    TextureCache textures;
    TextureCache cubeTextures;
    PipelineCache pipelines;
    SoundCache sounds;
    EffectCache effects;

    sq::PassConfigMap passConfigMap;

    vk::DescriptorSetLayout bindlessTextureSetLayout;
    vk::DescriptorSet bindlessTextureSet;

    void refresh_options();

private: //===============================================//

    uint32_t mNumBindlessTextures = 0u;

    sq::AudioContext& mAudioContext;
};

//============================================================================//

} // namespace sts
