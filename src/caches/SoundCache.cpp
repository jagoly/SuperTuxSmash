#include "caches/SoundCache.hpp"

#include <sqee/misc/Files.hpp>

using namespace sts;

SoundCache::SoundCache(sq::AudioContext& context) : mContext(context) {}

SoundCache::~SoundCache() = default;

std::unique_ptr<sq::Sound> SoundCache::create(const String& key)
{
    const String path = sq::compute_resource_path(key, {"assets/"}, {".wav", ".flac"});
    auto result = std::make_unique<sq::Sound>(mContext);
    result->load_from_file(path);
    return result;
}
