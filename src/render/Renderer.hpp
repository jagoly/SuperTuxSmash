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

//====== Forward Declarations ================================================//

namespace sts { struct HitBlob; struct HurtBlob; }
namespace sts { class DebugRender; class ParticleRender; }
namespace sts { class Camera; class RenderObject; }
namespace sts { class ParticleSet; }

//============================================================================//

namespace sts {

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const Options& options);

    ~Renderer();

    void refresh_options();

    //--------------------------------------------------------//

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    void update_from_scene_data(const SceneData& sceneData);

    //--------------------------------------------------------//

    void add_object(unique_ptr<RenderObject> object);

    //--------------------------------------------------------//

    void render_objects(float accum, float blend);

    void render_particles(const ParticleSet& particleSet, float accum, float blend);

    void render_blobs(const std::vector<HitBlob*>& blobs);

    void render_blobs(const std::vector<HurtBlob*>& blobs);

    void render_particles(const ParticleSet& particleSet);

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

        sq::Program Depth_SimpleSolid;
        sq::Program Depth_SkellySolid;
        sq::Program Depth_SimplePunch;
        sq::Program Depth_SkellyPunch;

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

private: //===================================================//

    unique_ptr<Camera> mCamera;

    std::vector<unique_ptr<RenderObject>> mRenderObjects;

    unique_ptr<DebugRender> mDebugRender;
    unique_ptr<ParticleRender> mParticleRender;
};

} // namespace sts
