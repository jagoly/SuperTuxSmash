#pragma once

#include <sqee/misc/ResourceCache.hpp>
#include <sqee/misc/ResourceHandle.hpp>

//====== Forward Declarations ================================================//

namespace sq {

class Texture2D;
class TextureArray2D;
class Mesh;

} // namespace sq

//============================================================================//

namespace sts {

//====== Alias Declarations ==================================================//

using TextureHandle  = sq::Handle<sq::Texture2D>;
using TexArrayHandle = sq::Handle<sq::TextureArray2D>;
using MeshHandle     = sq::Handle<sq::Mesh>;

//============================================================================//

class TextureCache final : public sq::ResourceCache<sq::Texture2D>
{
    public:  TextureCache(); ~TextureCache() override;
    private: unique_ptr<sq::Texture2D> create(const string& path) override;
};

//----------------------------------------------------------------------------//

class TexArrayCache final : public sq::ResourceCache<sq::TextureArray2D>
{
    public:  TexArrayCache(); ~TexArrayCache() override;
    private: unique_ptr<sq::TextureArray2D> create(const string& path) override;
};

//----------------------------------------------------------------------------//

class MeshCache final : public sq::ResourceCache<sq::Mesh>
{
    public:  MeshCache(); ~MeshCache() override;
    private: unique_ptr<sq::Mesh> create(const string& path) override;
};

//============================================================================//

class ResourceCaches final : sq::NonCopyable
{
public: //====================================================//

    ResourceCaches();
    ~ResourceCaches();

    //--------------------------------------------------------//

    TextureCache  textures;
    TexArrayCache texarrays;
    MeshCache     meshes;
};

//============================================================================//

} // namespace sts
