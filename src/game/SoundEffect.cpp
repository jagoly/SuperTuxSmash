#include "game/SoundEffect.hpp"

#include <sqee/misc/Json.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;

//============================================================================//

void SoundEffect::from_json(JsonObject json, SoundCache& cache)
{
    path = json["path"].as_auto();
    volume = json["volume"].as_auto();

    handle = cache.acquire(path);
}

//============================================================================//

void SoundEffect::to_json(JsonMutObject json) const
{
    json.append("path", path);
    json.append("volume", volume);
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool SoundEffect::operator==(const SoundEffect& other) const
{
    return path == other.path &&
           volume == other.volume;
}

ENABLE_WARNING_FLOAT_EQUALITY()
