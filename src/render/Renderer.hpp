#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include <sqee/app/PreProcessor.hpp>
#include <sqee/gl/FixedBuffer.hpp>
#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>

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

    sq::FrameBuffer FB_MsMain;
    sq::FrameBuffer FB_Resolve;

    sq::TextureMulti TEX_MsDepth;
    sq::TextureMulti TEX_MsColour;
    sq::Texture2D TEX_Depth;
    sq::Texture2D TEX_Colour;

    sq::Program PROG_Particles;
    sq::Program PROG_Lighting_Skybox;
    sq::Program PROG_Composite;

    //--------------------------------------------------------//

    // todo: These should be shared between all renderers, to reduce memory usage and loading time for the editor.
    //       We could share the fbos/textures/shaders above as well, unless we want a split viewports feature.

    sq::PreProcessor processor;

    MeshCache meshes;
    TextureCache textures;
    TexArrayCache texarrays;

    ProgramCache programs;
    MaterialCache materials;

    //--------------------------------------------------------//

    sq::Context& context;

    const Options& options;

private: //===================================================//

    sq::FixedBuffer mLightUbo;

    std::unique_ptr<Camera> mCamera;

    std::vector<std::unique_ptr<RenderObject>> mRenderObjects;

    std::unique_ptr<DebugRenderer> mDebugRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;
};

//============================================================================//

} // namespace sts
