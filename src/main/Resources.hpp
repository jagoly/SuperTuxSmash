#pragma once

#include <sqee/core/Types.hpp>

//#include <sqee/gl/Textures.hpp>
//#include <sqee/gl/Program.hpp>
//#include <sqee/objects/Material.hpp>
//#include <sqee/objects/Mesh.hpp>
//#include <sqee/objects/Sound.hpp>

#include <sqee/misc/ResourceCache.hpp>
#include <sqee/misc/ResourceHandle.hpp>

namespace sq {

class Texture2D;
class TextureArray;
class Program;
class Material;
class Mesh;
class Sound;

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

} // namespace sts
