#pragma once

#include "setup.hpp"

#include <sqee/objects/Mesh.hpp> // IWYU pragma: export

#include <sqee/misc/ResourceCache.hpp> // IWYU pragma: export
#include <sqee/misc/ResourceHandle.hpp> // IWYU pragma: export

namespace sts {

using MeshHandle = sq::Handle<sq::Mesh>;

class MeshCache final : public sq::ResourceCache<String, sq::Mesh>
{
public: //====================================================//

    MeshCache();

    ~MeshCache() override;

private: //===================================================//

    std::unique_ptr<sq::Mesh> create(const String& path) override;
};

} // namespace sts
