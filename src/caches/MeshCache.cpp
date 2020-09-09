#include "caches/MeshCache.hpp"

#include <sqee/misc/Files.hpp>

using namespace sts;

MeshCache::MeshCache() = default;

MeshCache::~MeshCache() = default;

std::unique_ptr<sq::Mesh> MeshCache::create(const String& key)
{
    const String path = sq::compute_resource_path(key, {"assets/"}, {".sqm"});
    auto result = std::make_unique<sq::Mesh>();
    result->load_from_file(path, true);
    return result;
}
