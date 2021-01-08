#include "game/SoundEffect.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Sound.hpp>

using namespace sts;

//============================================================================//

void SoundEffect::from_json(const JsonValue& json)
{
    SQASSERT(cache != nullptr, "");

    json.at("path").get_to(path);
    json.at("volume").get_to(volume);

    handle = cache->acquire(path.c_str());
}

//============================================================================//

void SoundEffect::to_json(JsonValue& json) const
{
    json["path"] = path;
    json["volume"] = volume;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool sts::operator==(const SoundEffect& a, const SoundEffect& b)
{
    if (a.path != b.path) return false;
    if (a.volume != b.volume) return false;

    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY()
