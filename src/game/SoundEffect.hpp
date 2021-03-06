#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

namespace sts {

//============================================================================//

struct SoundEffect final
{
    /// Must be set before calling from_json.
    SoundCache* cache = nullptr;

    /// Handle to the loaded sound resource.
    SoundHandle handle;

    /// ID of the sound when playing.
    int64_t id = -1;

    //--------------------------------------------------------//

    /// Path to the sound file to use.
    String path;

    /// Volume factor to apply to playback.
    float volume;

    //-- debug / editor methods ------------------------------//

    const TinyString& get_key() const
    {
        return *std::prev(reinterpret_cast<const TinyString*>(this));
    }

    //-- serialisation methods -------------------------------//

    void from_json(const JsonValue& json);
    void to_json(JsonValue& json) const;
};

//============================================================================//

bool operator==(const SoundEffect& a, const SoundEffect& b);

//============================================================================//

} // namespace sts
