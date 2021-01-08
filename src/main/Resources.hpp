#pragma once

#include "setup.hpp"

#include <sqee/misc/ResourceCache.hpp>
#include <sqee/misc/ResourceHandle.hpp>

namespace sq {

class Texture2D;
class TextureArray;
class Program;
class Material;
class Mesh;
class Sound;

class AudioContext;
class PreProcessor;

} // namespace sq

namespace sts {

using namespace sq::coretypes;

using TextureCache = sq::ResourceCache<String, sq::Texture2D>;
using TexArrayCache = sq::ResourceCache<String, sq::TextureArray>;
using ProgramCache = sq::ResourceCache<JsonValue, sq::Program>;
using MaterialCache = sq::ResourceCache<JsonValue, sq::Material>;
using MeshCache = sq::ResourceCache<String, sq::Mesh>;
using SoundCache = sq::ResourceCache<String, sq::Sound>;

using TextureHandle = sq::Handle<String, sq::Texture2D>;
using TexArrayHandle = sq::Handle<String, sq::TextureArray>;
using ProgramHandle = sq::Handle<JsonValue, sq::Program>;
using MaterialHandle = sq::Handle<JsonValue, sq::Material>;
using MeshHandle = sq::Handle<String, sq::Mesh>;
using SoundHandle = sq::Handle<String, sq::Sound>;

struct EffectAsset;
using EffectCache = sq::ResourceCache<String, EffectAsset>;
using EffectHandle = sq::Handle<String, EffectAsset>;

class ResourceCaches final : sq::NonCopyable
{
public: //================================================//

    ResourceCaches(sq::AudioContext& audio, sq::PreProcessor& processor);

    ~ResourceCaches();

    MeshCache meshes;
    TextureCache textures;
    TexArrayCache texarrays;

    ProgramCache programs;
    MaterialCache materials;

    SoundCache sounds;
    EffectCache effects;

private: //===============================================//

    sq::AudioContext& mAudioContext;
    sq::PreProcessor& mPreProcessor;
};

} // namespace sts
