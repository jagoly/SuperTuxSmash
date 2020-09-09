#pragma once

#include "setup.hpp"

#include <sqee/gl/Textures.hpp> // IWYU pragma: export

#include <sqee/misc/ResourceCache.hpp> // IWYU pragma: export
#include <sqee/misc/ResourceHandle.hpp> // IWYU pragma: export

namespace sts {

using TextureHandle = sq::Handle<sq::Texture2D>;

class TextureCache final : public sq::ResourceCache<String, sq::Texture2D>
{
public: //====================================================//

    TextureCache();

    ~TextureCache() override;

private: //===================================================//

    std::unique_ptr<sq::Texture2D> create(const String& path) override;
};

} // namespace sts
