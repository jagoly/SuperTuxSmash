#include "render/ResourceCaches.hpp"

#include <sqee/app/PreProcessor.hpp>

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

std::unique_ptr<sq::Texture2D> TextureCache::create(const String& path)
{
    auto result = std::make_unique<sq::Texture2D>();
    result->load_automatic(path);
    return result;
}

std::unique_ptr<sq::TextureArray2D> TexArrayCache::create(const String& path)
{
    auto result = std::make_unique<sq::TextureArray2D>();
    result->load_automatic(path);
    return result;
}

std::unique_ptr<sq::Mesh> MeshCache::create(const String& path)
{
    auto result = std::make_unique<sq::Mesh>();
    result->load_from_file(path, true);
    return result;
}

std::unique_ptr<sq::Program> ProgramCache::create(const sq::ProgramKey& key)
{
    auto result = std::make_unique<sq::Program>();
    mProcessor.load_vertex(*result, key.vertexPath, key.vertexDefines);
    mProcessor.load_fragment(*result, key.fragmentPath, key.fragmentDefines);
    result->link_program_stages();
    return result;
}

//============================================================================//

ResourceCaches::ResourceCaches(const sq::PreProcessor& processor)
    : textures(), texarrays(), meshes(), programs(processor) {}

ResourceCaches::~ResourceCaches() = default;
