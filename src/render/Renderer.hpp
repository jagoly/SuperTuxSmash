#pragma once

#include <sqee/app/PreProcessor.hpp>

#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

#include <sqee/render/Mesh.hpp>

#include "main/Options.hpp"
#include "render/ResourceCaches.hpp"
#include "render/SceneData.hpp"

#include "enumerations.hpp"

//====== Forward Declarations ================================================//

namespace sts { struct HitBlob; struct HurtBlob; }
namespace sts { class DebugRender; class ParticleRender; }
namespace sts { class Camera; class RenderObject; }
namespace sts { class ParticleSystem; }

//============================================================================//

namespace sts {

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(GameMode gameMode, const Options& options);

    ~Renderer();

    void refresh_options();

    //--------------------------------------------------------//

    Camera& get_camera() { return *mCamera; }

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    void add_object(UniquePtr<RenderObject> object);

    //--------------------------------------------------------//

    void render_objects(float accum, float blend);

    void render_particles(const ParticleSystem& system, float accum, float blend);

    void render_blobs(const Vector<HitBlob*>& blobs);

    void render_blobs(const Vector<HurtBlob*>& blobs);

    void finish_rendering();

    //--------------------------------------------------------//

    struct {

        sq::FrameBuffer Depth;
        sq::FrameBuffer Main;
        sq::FrameBuffer Resolve;
        //sq::FrameBuffer Final;

    } fbos;

    //--------------------------------------------------------//

    struct {

        sq::TextureMulti Depth { sq::Texture::Format::DEP24S8 };
        sq::TextureMulti Colour { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Resolve { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Final { sq::Texture::Format::RGBA8_UN };

    } textures;

    //--------------------------------------------------------//

    struct {

        sq::Program Depth_StaticSolid;
        sq::Program Depth_StaticPunch;

        sq::Program Depth_FighterSolid;
        sq::Program Depth_FighterPunch;

        sq::Program Particles;

        sq::Program Lighting_Skybox;
        sq::Program Composite;

    } shaders;

    //--------------------------------------------------------//

    struct { sq::UniformBuffer ubo; } light;

    //--------------------------------------------------------//

    ResourceCaches resources;

    sq::PreProcessor processor;

    sq::Context& context;

    const Options& options;

    const GameMode gameMode;

private: //===================================================//

    UniquePtr<Camera> mCamera;

    Vector<UniquePtr<RenderObject>> mRenderObjects;

    UniquePtr<DebugRender> mDebugRender;
    UniquePtr<ParticleRender> mParticleRender;
};

} // namespace sts
