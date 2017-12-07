#include <sqee/debug/Logging.hpp>

#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>

#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"
#include "render/Camera.hpp"
#include "render/RenderObject.hpp"

#include "render/Renderer.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Renderer::~Renderer() = default;

Renderer::Renderer(const Options& options) : context(sq::Context::get()), options(options)
{
    //-- Set Texture Paramaters ------------------------------//

    textures.Colour.set_filter_mode(true);
    textures.Final.set_filter_mode(true);

    //-- Import GLSL Headers ---------------------------------//

    processor.import_header("headers/blocks/Camera");
    processor.import_header("headers/blocks/Light");
    processor.import_header("headers/blocks/Skeleton");

    processor.import_header("headers/super/Simple_vs");
    processor.import_header("headers/super/Skelly_vs");
    processor.import_header("headers/super/Model_fs");

    //-- Create Uniform Buffers ------------------------------//

    light.ubo.create_and_allocate(112u);

    //--------------------------------------------------------//

    mCamera = std::make_unique<Camera>(*this);

    mDebugRender = std::make_unique<DebugRender>(*this);
    mParticleRender = std::make_unique<ParticleRender>(*this);
}

//============================================================================//

void Renderer::refresh_options()
{
    //-- Prepare Shader Options Header -----------------------//

    string headerStr = "// set of constants and defines added at runtime\n";

    headerStr += "const uint OPTION_WinWidth  = " + std::to_string(options.Window_Size.x) + ";\n";
    headerStr += "const uint OPTION_WinHeight = " + std::to_string(options.Window_Size.y) + ";\n";

    if (options.Bloom_Enable == true) headerStr += "#define OPTION_BLOOM_ENABLE\n";;
    if (options.SSAO_Quality != 0u)   headerStr += "#define OPTION_SSAO_ENABLE\n";
    if (options.SSAO_Quality >= 2u)   headerStr += "#define OPTION_SSAO_HIGH\n";

    headerStr += "// some handy shortcuts for comman use of this data\n"
                 "const float OPTION_Aspect = float(OPTION_WinWidth) / float(OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeFull = vec2(OPTION_WinWidth, OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeHalf = round(OPTION_WinSizeFull / 2.f);\n"
                 "const vec2 OPTION_WinSizeQter = round(OPTION_WinSizeFull / 4.f);\n";

    processor.update_header("runtime/Options", headerStr);

    //-- Allocate Target Textures ----------------------------//

    const uint msaaNum = std::max(4u * options.MSAA_Quality * options.MSAA_Quality, 1u);

    textures.Depth.allocate_storage(Vec3U(options.Window_Size, msaaNum));
    textures.Colour.allocate_storage(Vec3U(options.Window_Size, msaaNum));

    textures.Resolve.allocate_storage(options.Window_Size);
    //textures.Final.allocate_storage(options.Window_Size);

    //-- Attach Textures to FrameBuffers ---------------------//

    fbos.Depth.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);
    fbos.Main.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);
    fbos.Main.attach(gl::COLOR_ATTACHMENT0, textures.Colour);
    fbos.Resolve.attach(gl::COLOR_ATTACHMENT0, textures.Resolve);
    //fbos.Final.attach(gl::COLOR_ATTACHMENT0, textures.Final);

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(shaders.Depth_SimpleSolid, "depth/Simple_vs");
    processor.load_vertex(shaders.Depth_SkellySolid, "depth/Skelly_vs");
    processor.load_vertex(shaders.Depth_SimplePunch, "depth/Simple_vs");
    processor.load_vertex(shaders.Depth_SkellyPunch, "depth/Skelly_vs");

    processor.load_fragment(shaders.Depth_SimplePunch, "depth/Mask_fs");
    processor.load_fragment(shaders.Depth_SkellyPunch, "depth/Mask_fs");

    processor.load_vertex(shaders.Particles, "particles/test_vs");
    processor.load_fragment(shaders.Particles, "particles/test_fs");

    processor.load_vertex(shaders.Lighting_Skybox, "lighting/Skybox_vs");
    processor.load_vertex(shaders.Composite, "FullScreen_vs");

    processor.load_fragment(shaders.Lighting_Skybox, "lighting/Skybox_fs");
    processor.load_fragment(shaders.Composite, "Composite_fs");

    //-- Link Shader Program Stages --------------------------//

    shaders.Depth_SimpleSolid.link_program_stages();
    shaders.Depth_SkellySolid.link_program_stages();
    shaders.Depth_SimplePunch.link_program_stages();
    shaders.Depth_SkellyPunch.link_program_stages();

    shaders.Particles.link_program_stages();

    shaders.Lighting_Skybox.link_program_stages();
    shaders.Composite.link_program_stages();

    //--------------------------------------------------------//

    mDebugRender->refresh_options();
    mParticleRender->refresh_options();
}

//============================================================================//

void Renderer::add_object(unique_ptr<RenderObject> object)
{
    mRenderObjects.push_back(std::move(object));
}

void Renderer::update_from_scene_data(const SceneData& sceneData)
{
    mCamera->update_from_scene_data(sceneData);
}

//============================================================================//

namespace { // anonymous

struct StaticShit
{
    StaticShit()
    {
        TEX_Skybox.load_automatic("skybox");
    }

    sq::TextureCube TEX_Skybox;
};

} // anonymous namespace

//============================================================================//

void Renderer::render_objects(float elapsed, float blend)
{
    static StaticShit shit;

    //-- Update the Camera -----------------------------------//

    mCamera->intergrate(blend);

    //-- Update the Lighting ---------------------------------//

    const Vec3F skyDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));
    const Vec3F ambiColour = { 0.5f, 0.5f, 0.5f };
    const Vec3F skyColour = { 0.7f, 0.7f, 0.7f };
    const Mat4F skyMatrix = Mat4F();

    light.ubo.update_complete(ambiColour, 0, skyColour, 0, skyDirection, 0, skyMatrix);

    //-- Integrate Object Changes ----------------------------//

    for (const auto& object : mRenderObjects)
        object->integrate(blend);

    //-- Setup Shared Rendering State ------------------------//

    context.set_ViewPort(options.Window_Size);

    context.bind_UniformBuffer(mCamera->get_ubo(), 0u);
    context.bind_UniformBuffer(light.ubo, 1u);

    //-- Clear the FrameBuffer -------------------------------//

    context.bind_FrameBuffer(fbos.Main);

    context.clear_Colour({0.f, 0.f, 0.f, 0.f});
    context.clear_Depth_Stencil();

    //-- Render the Skybox -----------------------------------//

    context.set_state(Context::Blend_Mode::Disable);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Texture(shit.TEX_Skybox, 0u);
    context.bind_Program(shaders.Lighting_Skybox);

    sq::draw_screen_quad();

    //-- Render Depth Pass -----------------------------------//

    context.bind_FrameBuffer(fbos.Depth);

    for (const auto& object : mRenderObjects)
        object->render_depth();

    //-- Render Main Pass ------------------------------------//

    context.bind_FrameBuffer(fbos.Main);

    for (const auto& object : mRenderObjects)
        object->render_main();

    //-- Render Alpha Pass -----------------------------------//

    context.bind_FrameBuffer(fbos.Main);

    for (const auto& object : mRenderObjects)
        object->render_alpha();
}

//============================================================================//

void Renderer::render_particles(float accum, float blend)
{
    mParticleRender->swap_sets();

    for (const auto& object : mRenderObjects)
        for (const auto& set : object->get_particle_sets())
            mParticleRender->integrate_set(blend, set);

    mParticleRender->render_particles();
}

//============================================================================//

void Renderer::finish_rendering()
{
    //-- Resolve the Multi Sample Texture --------------------//

    fbos.Main.blit(fbos.Resolve, options.Window_Size, gl::COLOR_BUFFER_BIT);

    //-- Composite to the Default Framebuffer ----------------//

    context.bind_FrameBuffer_default();

    context.set_state(Context::Blend_Mode::Disable);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Texture(textures.Resolve, 0u);
    context.bind_Program(shaders.Composite);

    sq::draw_screen_quad();
}

//============================================================================//

void Renderer::render_blobs(const std::vector<HitBlob*>& blobs)
{
    mDebugRender->render_blobs(blobs);
}

void Renderer::render_blobs(const std::vector<HurtBlob*>& blobs)
{
    mDebugRender->render_blobs(blobs);
}
