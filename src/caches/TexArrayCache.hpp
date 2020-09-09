#pragma once

#include "setup.hpp"

#include <sqee/gl/Textures.hpp> // IWYU pragma: export

#include <sqee/misc/ResourceCache.hpp> // IWYU pragma: export
#include <sqee/misc/ResourceHandle.hpp> // IWYU pragma: export

namespace sts {

using TexArrayHandle = sq::Handle<sq::TextureArray2D>;

class TexArrayCache final : public sq::ResourceCache<String, sq::TextureArray2D>
{
public: //====================================================//

    TexArrayCache();

    ~TexArrayCache() override;

private: //===================================================//

    std::unique_ptr<sq::TextureArray2D> create(const String& path) override;
};

} // namespace sts
