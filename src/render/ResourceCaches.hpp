#pragma once

#include "setup.hpp"

#include <sqee/gl/Program.hpp> // IWYU pragma: export
#include <sqee/gl/Textures.hpp> // IWYU pragma: export
#include <sqee/objects/Mesh.hpp> // IWYU pragma: export

#include <sqee/misc/ResourceCache.hpp> // IWYU pragma: export
#include <sqee/misc/ResourceHandle.hpp> // IWYU pragma: export

//============================================================================//

namespace sq { class PreProcessor; }

namespace sts {

//====== Alias Declarations ==================================================//

using TextureHandle   = sq::Handle<String, sq::Texture2D>;
using TexArrayHandle  = sq::Handle<String, sq::TextureArray2D>;
using MeshHandle      = sq::Handle<String, sq::Mesh>;
using ProgramHandle   = sq::Handle<sq::ProgramKey, sq::Program>;

//============================================================================//

class TextureCache final : public sq::ResourceCache<String, sq::Texture2D>
{
public:
    TextureCache();
    ~TextureCache() override;
private:
    std::unique_ptr<sq::Texture2D> create(const String& path) override;
};

//----------------------------------------------------------------------------//

class TexArrayCache final : public sq::ResourceCache<String, sq::TextureArray2D>
{
public:
    TexArrayCache();
    ~TexArrayCache() override;
private:
    std::unique_ptr<sq::TextureArray2D> create(const String& path) override;
};

//----------------------------------------------------------------------------//

class MeshCache final : public sq::ResourceCache<String, sq::Mesh>
{
public:
    MeshCache();
    ~MeshCache() override;
private:
    std::unique_ptr<sq::Mesh> create(const String& path) override;
};

//----------------------------------------------------------------------------//

class ProgramCache final : public sq::ResourceCache<sq::ProgramKey, sq::Program>
{
public:
    ProgramCache(const sq::PreProcessor& processor);
    ~ProgramCache() override;
private:
    std::unique_ptr<sq::Program> create(const sq::ProgramKey& key) override;
    const sq::PreProcessor& mProcessor;
};

//============================================================================//

class ResourceCaches final : sq::NonCopyable
{
public: //====================================================//

    ResourceCaches(const sq::PreProcessor& processor);
    ~ResourceCaches();

    TextureCache  textures;
    TexArrayCache texarrays;
    MeshCache     meshes;
    ProgramCache  programs;
};

//============================================================================//

} // namespace sts
