#include "caches/TextureCache.hpp"

#include <sqee/misc/Files.hpp>

using namespace sts;

TextureCache::TextureCache() = default;

TextureCache::~TextureCache() = default;

std::unique_ptr<sq::Texture2D> TextureCache::create(const String& key)
{
    const String path = sq::compute_resource_path(key, {"assets/"}, {".png"});
    auto result = std::make_unique<sq::Texture2D>();
    result->load_automatic(path);
    return result;
}
