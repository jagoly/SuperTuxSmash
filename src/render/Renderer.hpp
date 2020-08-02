#pragma once

#include "setup.hpp"

#include "render/ResourceCaches.hpp"

#include <sqee/app/PreProcessor.hpp>
#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

//============================================================================//

namespace sq { class Context; }

namespace sts {

class DebugRenderer;
class ParticleRenderer;

//============================================================================//

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const Options& options);

    ~Renderer();

    void refresh_options();

    //--------------------------------------------------------//

    void set_camera(std::unique_ptr<Camera> camera);

    Camera& get_camera() { return *mCamera; }

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    void add_object(std::unique_ptr<RenderObject> object);

    std::unique_ptr<RenderObject> remove_object(RenderObject* ptr);

    //--------------------------------------------------------//

    void render_objects(float blend);

    void render_particles(const ParticleSystem& system, float blend);

    void resolve_multisample();

    void finish_rendering();

    //--------------------------------------------------------//

    DebugRenderer& get_debug_renderer() { return *mDebugRenderer; }

    //--------------------------------------------------------//

    struct {

        sq::FrameBuffer MsDepth;
        sq::FrameBuffer MsMain;
        sq::FrameBuffer Resolve;

    } fbos;

    //--------------------------------------------------------//

    struct {

        sq::TextureMulti MsDepth { sq::Texture::Format::DEP24S8 };
        sq::TextureMulti MsColour { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Depth { sq::Texture::Format::DEP24S8 };
        sq::Texture2D Colour { sq::Texture::Format::RGB16_FP };

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

private: //===================================================//

    std::unique_ptr<Camera> mCamera;

    std::vector<std::unique_ptr<RenderObject>> mRenderObjects;

    std::unique_ptr<DebugRenderer> mDebugRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;
};

//============================================================================//

} // namespace sts
