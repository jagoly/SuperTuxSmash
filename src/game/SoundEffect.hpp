#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

namespace sts {

//============================================================================//

struct SoundEffect final
{
    /// Handle to the loaded sq::Sound resource.
    SoundHandle handle = nullptr;

    //--------------------------------------------------------//

    /// Path to the sound file to use.
    String path = {};

    /// Volume factor to apply to playback.
    float volume = 100.f;

    //--------------------------------------------------------//

    const SmallString& get_key() const
    {
        return *std::prev(reinterpret_cast<const SmallString*>(this));
    }

    void from_json(const JsonValue& json, SoundCache& cache);

    void to_json(JsonValue& json) const;

    bool operator==(const SoundEffect& other) const;
};

//============================================================================//

} // namespace sts
