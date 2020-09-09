#include "caches/TexArrayCache.hpp"

using namespace sts;

TexArrayCache::TexArrayCache() = default;

TexArrayCache::~TexArrayCache() = default;

std::unique_ptr<sq::TextureArray2D> TexArrayCache::create(const String& path)
{
    auto result = std::make_unique<sq::TextureArray2D>();
    result->load_automatic(path);
    return result;
}
