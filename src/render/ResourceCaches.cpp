#include <sqee/app/PreProcessor.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/render/Mesh.hpp>
#include <sqee/render/Armature.hpp>

#include "ResourceCaches.hpp"

using namespace sts;

//============================================================================//

TextureCache::TextureCache() = default;

TexArrayCache::TexArrayCache() = default;

MeshCache::MeshCache() = default;

ProgramCache::ProgramCache(const sq::PreProcessor& p) : mProcessor(p) {}

//============================================================================//

TextureCache::~TextureCache() = default;

TexArrayCache::~TexArrayCache() = default;

MeshCache::~MeshCache() = default;

ProgramCache::~ProgramCache() = default;

//============================================================================//

UniquePtr<sq::Texture2D> TextureCache::create(const String& path)
{
    auto result = std::make_unique<sq::Texture2D>();
    result->load_automatic(path);
    return result;
}

UniquePtr<sq::TextureArray2D> TexArrayCache::create(const String& path)
{
    auto result = std::make_unique<sq::TextureArray2D>();
    result->load_automatic(path);
    return result;
}

UniquePtr<sq::Mesh> MeshCache::create(const String& path)
{
    auto result = std::make_unique<sq::Mesh>();
    result->load_from_file(path, true);
    return result;
}

UniquePtr<sq::Program> ProgramCache::create(const sq::ProgramKey& key)
{
    auto result = std::make_unique<sq::Program>();
    mProcessor.load_vertex(*result, key.vertexPath, key.vertexDefines);
    mProcessor.load_fragment(*result, key.fragmentPath, key.fragmentDefines);
    result->link_program_stages();
    return result;
}

//============================================================================//

ResourceCaches::ResourceCaches(const sq::PreProcessor& p) : textures(), texarrays(), meshes(), programs(p) {}

ResourceCaches::~ResourceCaches() = default;
