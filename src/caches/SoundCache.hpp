#pragma once

#include "setup.hpp"

#include <sqee/objects/Sound.hpp> // IWYU pragma: export

#include <sqee/misc/ResourceCache.hpp> // IWYU pragma: export
#include <sqee/misc/ResourceHandle.hpp> // IWYU pragma: export

namespace sq { class AudioContext; }

namespace sts {

using SoundHandle = sq::Handle<sq::Sound>;

class SoundCache final : public sq::ResourceCache<String, sq::Sound>
{
public: //====================================================//

    SoundCache(sq::AudioContext& context);

    ~SoundCache() override;

private: //===================================================//

    std::unique_ptr<sq::Sound> create(const String& path) override;

    sq::AudioContext& mContext;
};

} // namespace sts
