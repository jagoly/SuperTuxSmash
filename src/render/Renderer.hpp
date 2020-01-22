#pragma once

#include <sqee/app/PreProcessor.hpp>

#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

#include <sqee/render/Mesh.hpp>

#include "main/Enumerations.hpp"
#include "main/Globals.hpp"
#include "main/Options.hpp"

#include "render/ResourceCaches.hpp"
#include "render/SceneData.hpp"

//====== Forward Declarations ================================================//

namespace sts { struct HitBlob; struct HurtBlob; }
namespace sts { class DebugRenderer; class ParticleRenderer; }
namespace sts { class Camera; class RenderObject; }
namespace sts { class ParticleSystem; }

//============================================================================//

namespace sts {

//============================================================================//

struct DebugArrow { Vec2F origin; float angle; float length; };

//============================================================================//

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const Globals& globals, const Options& options);

    ~Renderer();

    void refresh_options();

    //--------------------------------------------------------//

    Camera& get_camera() { return *mCamera; }

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    void add_object(UniquePtr<RenderObject> object);

    UniquePtr<RenderObject> remove_object(RenderObject* ptr);

    //--------------------------------------------------------//

    void render_objects(float accum, float blend);

    void render_particles(const ParticleSystem& system, float accum, float blend);

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

    const Globals& globals;

    const Options& options;

private: //===================================================//

    UniquePtr<Camera> mCamera;

    Vector<UniquePtr<RenderObject>> mRenderObjects;

    UniquePtr<DebugRenderer> mDebugRenderer;
    UniquePtr<ParticleRenderer> mParticleRenderer;
};

} // namespace sts
