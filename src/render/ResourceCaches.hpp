#pragma once

#include <sqee/misc/Builtins.hpp>

#include <sqee/misc/ResourceCache.hpp>
#include <sqee/misc/ResourceHandle.hpp>

//====== Forward Declarations ================================================//

namespace sq {

class Texture2D;
class TextureArray2D;
class Mesh;

class Program;
struct ProgramKey;
class PreProcessor;

} // namespace sq

//============================================================================//

namespace sts {

//====== Alias Declarations ==================================================//

using TextureHandle  = sq::Handle<String, sq::Texture2D>;
using TexArrayHandle = sq::Handle<String, sq::TextureArray2D>;
using MeshHandle     = sq::Handle<String, sq::Mesh>;
using ProgramHandle  = sq::Handle<sq::ProgramKey, sq::Program>;

//============================================================================//

class TextureCache final : public sq::ResourceCache<String, sq::Texture2D>
{
public:
    TextureCache();
    ~TextureCache() override;
private:
    UniquePtr<sq::Texture2D> create(const String& path) override;
};

//----------------------------------------------------------------------------//

class TexArrayCache final : public sq::ResourceCache<String, sq::TextureArray2D>
{
public:
    TexArrayCache();
    ~TexArrayCache() override;
private:
    UniquePtr<sq::TextureArray2D> create(const String& path) override;
};

//----------------------------------------------------------------------------//

class MeshCache final : public sq::ResourceCache<String, sq::Mesh>
{
public:
    MeshCache();
    ~MeshCache() override;
private:
    UniquePtr<sq::Mesh> create(const String& path) override;
};

//----------------------------------------------------------------------------//

class ProgramCache final : public sq::ResourceCache<sq::ProgramKey, sq::Program>
{
public:
    ProgramCache(const sq::PreProcessor& processor);
    ~ProgramCache() override;
private:
    UniquePtr<sq::Program> create(const sq::ProgramKey& key) override;
    const sq::PreProcessor& mProcessor;
};

//============================================================================//

class ResourceCaches final : sq::NonCopyable
{
public: //====================================================//

    ResourceCaches(const sq::PreProcessor&);
    ~ResourceCaches();

    //--------------------------------------------------------//

    TextureCache  textures;
    TexArrayCache texarrays;
    MeshCache     meshes;
    ProgramCache  programs;
};

//============================================================================//

} // namespace sts
