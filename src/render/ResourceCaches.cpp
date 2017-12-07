#include <sqee/gl/Textures.hpp>
#include <sqee/render/Mesh.hpp>
#include <sqee/render/Armature.hpp>

#include "ResourceCaches.hpp"

using namespace sts;

//============================================================================//

TextureCache::TextureCache() = default;

TexArrayCache::TexArrayCache() = default;

MeshCache::MeshCache() = default;

//============================================================================//

TextureCache::~TextureCache() = default;

TexArrayCache::~TexArrayCache() = default;

MeshCache::~MeshCache() = default;

//============================================================================//

unique_ptr<sq::Texture2D> TextureCache::create(const string& path)
{
    auto result = std::make_unique<sq::Texture2D>();
    result->load_automatic(path);
    return result;
}

unique_ptr<sq::TextureArray2D> TexArrayCache::create(const string& path)
{
    auto result = std::make_unique<sq::TextureArray2D>();
    result->load_automatic(path);
    return result;
}

unique_ptr<sq::Mesh> MeshCache::create(const string& path)
{
    auto result = std::make_unique<sq::Mesh>();
    result->load_from_file(path, true);
    return result;
}

//============================================================================//

ResourceCaches::ResourceCaches() = default;

ResourceCaches::~ResourceCaches() = default;
