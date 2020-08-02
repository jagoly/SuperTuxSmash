#include "render/Renderer.hpp"

#include "main/Options.hpp"

#include "render/Camera.hpp"
#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"
#include "render/RenderObject.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>
#include <sqee/redist/gl_loader.hpp>

using sq::Context;
using namespace sts;

//============================================================================//

Renderer::Renderer(const Options& options)
    : resources(processor)
    , context(sq::Context::get())
    , options(options)
{
    //-- Set Texture Paramaters ------------------------------//

    textures.MsColour.set_filter_mode(true);
    textures.Colour.set_filter_mode(true);

    //-- Import GLSL Headers ---------------------------------//

    processor.import_header("headers/blocks/Camera");
    processor.import_header("headers/blocks/Fighter");
    processor.import_header("headers/blocks/Light");

    //-- Create Uniform Buffers ------------------------------//

    light.ubo.create_and_allocate(112u);

    //--------------------------------------------------------//

    mDebugRenderer = std::make_unique<DebugRenderer>(*this);
    mParticleRenderer = std::make_unique<ParticleRenderer>(*this);
}

Renderer::~Renderer() = default;

//============================================================================//

void Renderer::refresh_options()
{
    //-- Prepare Shader Options Header -----------------------//

    String headerStr = "// set of constants and defines added at runtime\n";

    headerStr += "const uint OPTION_WinWidth  = " + std::to_string(options.window_size.x) + ";\n";
    headerStr += "const uint OPTION_WinHeight = " + std::to_string(options.window_size.y) + ";\n";

    if (options.bloom_enable == true) headerStr += "#define OPTION_BLOOM_ENABLE\n";;
    if (options.ssao_quality != 0u)   headerStr += "#define OPTION_SSAO_ENABLE\n";
    if (options.ssao_quality >= 2u)   headerStr += "#define OPTION_SSAO_HIGH\n";

    headerStr += "// some handy shortcuts for comman use of this data\n"
                 "const float OPTION_Aspect = float(OPTION_WinWidth) / float(OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeFull = vec2(OPTION_WinWidth, OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeHalf = round(OPTION_WinSizeFull / 2.f);\n"
                 "const vec2 OPTION_WinSizeQter = round(OPTION_WinSizeFull / 4.f);\n";

    processor.update_header("runtime/Options", headerStr);

    //-- Allocate Target Textures ----------------------------//

    const uint msaaNum = std::max(4u * options.msaa_quality * options.msaa_quality, 1u);

    textures.MsDepth.allocate_storage(Vec3U(options.window_size, msaaNum));
    textures.MsColour.allocate_storage(Vec3U(options.window_size, msaaNum));

    textures.Depth.allocate_storage(options.window_size);
    textures.Colour.allocate_storage(options.window_size);

    //-- Attach Textures to FrameBuffers ---------------------//

    fbos.MsDepth.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.MsDepth);

    fbos.MsMain.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.MsDepth);
    fbos.MsMain.attach(gl::COLOR_ATTACHMENT0, textures.MsColour);

    fbos.Resolve.attach(gl::DEPTH_ATTACHMENT, textures.Depth);
    fbos.Resolve.attach(gl::COLOR_ATTACHMENT0, textures.Colour);

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(shaders.Depth_StaticSolid, "depth/Static_vs");
    processor.load_vertex(shaders.Depth_StaticPunch, "depth/Static_vs");
    processor.load_fragment(shaders.Depth_StaticPunch, "depth/Mask_fs");

    processor.load_vertex(shaders.Depth_FighterSolid, "depth/Fighter_vs");
    processor.load_vertex(shaders.Depth_FighterPunch, "depth/Fighter_vs");
    processor.load_fragment(shaders.Depth_FighterPunch, "depth/Mask_fs");

    processor.load_vertex(shaders.Particles, "particles/test_vs");
    processor.load_geometry(shaders.Particles, "particles/test_gs");
    processor.load_fragment(shaders.Particles, "particles/test_fs");

    processor.load_vertex(shaders.Lighting_Skybox, "lighting/Skybox_vs");
    processor.load_fragment(shaders.Lighting_Skybox, "lighting/Skybox_fs");

    processor.load_vertex(shaders.Composite, "FullScreen_vs");
    processor.load_fragment(shaders.Composite, "Composite_fs");

    //-- Link Shader Program Stages --------------------------//

    shaders.Depth_StaticSolid.link_program_stages();
    shaders.Depth_StaticPunch.link_program_stages();

    shaders.Depth_FighterSolid.link_program_stages();
    shaders.Depth_FighterPunch.link_program_stages();

    shaders.Particles.link_program_stages();

    shaders.Lighting_Skybox.link_program_stages();
    shaders.Composite.link_program_stages();

    //--------------------------------------------------------//

    mDebugRenderer->refresh_options();
    mParticleRenderer->refresh_options();
}

//============================================================================//

void Renderer::set_camera(std::unique_ptr<Camera> camera)
{
    mCamera = std::move(camera);
}

void Renderer::add_object(std::unique_ptr<RenderObject> object)
{
    mRenderObjects.push_back(std::move(object));
}

std::unique_ptr<RenderObject> Renderer::remove_object(RenderObject* ptr)
{
    const auto predicate = [ptr](auto& item) { return item.get() == ptr; };
    const auto iter = algo::find_if(mRenderObjects, predicate);
    SQASSERT(iter != mRenderObjects.end(), "invalid ptr for remove");
    auto result = std::move(*iter);
    mRenderObjects.erase(iter);
    return result;
}

//============================================================================//

namespace { // anonymous

struct StaticShit
{
    StaticShit()
    {
        TEX_Skybox.load_automatic("assets/skybox");
    }

    sq::TextureCube TEX_Skybox;
};

} // anonymous namespace

//============================================================================//

void Renderer::render_objects(float blend)
{
    static StaticShit shit;

    //-- Update the Camera -----------------------------------//

    mCamera->intergrate(blend);

    //-- Update the Lighting ---------------------------------//

    const Vec3F skyDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));
    const Vec3F ambiColour = { 0.5f, 0.5f, 0.5f };
    const Vec3F skyColour = { 0.7f, 0.7f, 0.7f };
    const Mat4F skyMatrix = Mat4F();

    light.ubo.update(0u, sq::Structure(ambiColour, 0, skyColour, 0, skyDirection, 0, skyMatrix));

    //-- Integrate Object Changes ----------------------------//

    for (const auto& object : mRenderObjects)
        object->integrate(blend);

    //-- Setup Shared Rendering State ------------------------//

    context.set_ViewPort(options.window_size);

    context.bind_UniformBuffer(mCamera->get_ubo(), 0u);
    context.bind_UniformBuffer(light.ubo, 1u);

    //-- Clear the FrameBuffer -------------------------------//

    context.bind_FrameBuffer(fbos.MsMain);

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

    context.bind_FrameBuffer(fbos.MsDepth);

    for (const auto& object : mRenderObjects)
        object->render_depth();

    //-- Render Main Pass ------------------------------------//

    context.bind_FrameBuffer(fbos.MsMain);

    for (const auto& object : mRenderObjects)
        object->render_main();

    //-- Render Alpha Pass -----------------------------------//

    context.bind_FrameBuffer(fbos.MsMain);

    for (const auto& object : mRenderObjects)
        object->render_alpha();
}

//============================================================================//

void Renderer::render_particles(const ParticleSystem& system, float blend)
{
    mParticleRenderer->swap_sets();

    mParticleRenderer->integrate_set(blend, system);

    mParticleRenderer->render_particles();
}

//============================================================================//

void Renderer::resolve_multisample()
{
    //-- Resolve the Multi Sample Texture --------------------//

    fbos.MsMain.blit(fbos.Resolve, options.window_size, gl::DEPTH_BUFFER_BIT | gl::COLOR_BUFFER_BIT);
}

//============================================================================//

void Renderer::finish_rendering()
{
    //-- Composite to the Default Framebuffer ----------------//

    context.bind_FrameBuffer_default();

    context.set_state(Context::Blend_Mode::Disable);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Texture(textures.Colour, 0u);
    context.bind_Program(shaders.Composite);

    sq::draw_screen_quad();
}
