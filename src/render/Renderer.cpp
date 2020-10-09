#include "render/Renderer.hpp"

#include "main/Options.hpp"

#include "render/Camera.hpp"
#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"
#include "render/RenderObject.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/gl/Context.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>

using namespace sts;

//============================================================================//

Renderer::Renderer(const Options& options) : context(sq::Context::get()), options(options)
{
    meshes.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::Mesh>();
        result->load_from_file(sq::build_string("assets/", key, ".sqm"), true);
        return result;
    });

    textures.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::Texture2D>();
        result->load_automatic(sq::build_string("assets/", key));
        return result;
    });

    texarrays.assign_factory([](const String& key)
    {
        auto result = std::make_unique<sq::TextureArray>();
        result->load_automatic(sq::build_string("assets/", key));
        return result;
    });

    programs.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::Program>();
        processor.load_super_shader(*result, sq::build_string("shaders/", key.at("path"), ".glsl"), key.at("options"));
        return result;
    });

    materials.assign_factory([this](const JsonValue& key)
    {
        auto result = std::make_unique<sq::Material>();
        result->load_from_json(key, programs, textures);
        return result;
    });

    //-- Import GLSL Headers ---------------------------------//

    processor.import_header("headers/blocks/Camera");
    processor.import_header("headers/blocks/Light");
    processor.import_header("headers/blocks/Skelly");
    processor.import_header("headers/blocks/Static");

    //-- Create Uniform Buffers ------------------------------//

    mLightUbo.allocate_dynamic(112u);

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

    processor.import_header("runtime/Options", headerStr);

    //-- Allocate Target Textures ----------------------------//

    const uint msaaNum = std::max(4u * options.msaa_quality * options.msaa_quality, 1u);

    TEX_MsDepth = sq::TextureMulti();
    TEX_MsDepth.allocate_storage(sq::TexFormat::DEP24S8, options.window_size, msaaNum);

    TEX_MsColour = sq::TextureMulti();
    TEX_MsColour.allocate_storage(sq::TexFormat::RGB16_FP, options.window_size, msaaNum);

    TEX_Depth = sq::Texture2D();
    TEX_Depth.allocate_storage(sq::TexFormat::DEP24S8, options.window_size, false);

    TEX_Colour = sq::Texture2D();
    TEX_Colour.allocate_storage(sq::TexFormat::RGB16_FP, options.window_size, false);

    //-- Attach Textures to FrameBuffers ---------------------//

    FB_MsMain.attach(sq::FboAttach::DepthStencil, TEX_MsDepth);
    FB_MsMain.attach(sq::FboAttach::Colour0, TEX_MsColour);

    FB_Resolve.attach(sq::FboAttach::Depth, TEX_Depth);
    FB_Resolve.attach(sq::FboAttach::Colour0, TEX_Colour);

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(PROG_Particles, "shaders/particles/test_vs.glsl", {});
    processor.load_geometry(PROG_Particles, "shaders/particles/test_gs.glsl", {});
    processor.load_fragment(PROG_Particles, "shaders/particles/test_fs.glsl", {});

    processor.load_vertex(PROG_Lighting_Skybox, "shaders/stage/Skybox_vs.glsl", {});
    processor.load_fragment(PROG_Lighting_Skybox, "shaders/stage/Skybox_fs.glsl", {});

    processor.load_vertex(PROG_Composite, "shaders/FullScreen_vs.glsl", {});
    processor.load_fragment(PROG_Composite, "shaders/Composite_fs.glsl", {});

    //-- Link Shader Program Stages --------------------------//

    PROG_Particles.link_program_stages();

    PROG_Lighting_Skybox.link_program_stages();
    PROG_Composite.link_program_stages();

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

    mLightUbo.update(0u, sq::Structure(ambiColour, 0, skyColour, 0, skyDirection, 0, skyMatrix));

    //-- Integrate Object Changes ----------------------------//

    for (const auto& object : mRenderObjects)
        object->integrate(blend);

    //-- Setup Shared Rendering State ------------------------//

    context.set_ViewPort(options.window_size);

    context.bind_buffer(mCamera->get_ubo(), sq::BufTarget::Uniform, 0u);
    context.bind_buffer(mLightUbo, sq::BufTarget::Uniform, 1u);

    //-- Clear the FrameBuffer -------------------------------//

    context.bind_framebuffer(FB_MsMain);

    context.clear_depth_stencil_colour(1.0, 0x00, 0xFF, Vec4F(0.f));

    //-- Render the Skybox -----------------------------------//

    context.set_state(sq::BlendMode::Disable);
    context.set_state(sq::CullFace::Disable);
    context.set_state(sq::DepthTest::Disable);

    context.bind_texture(shit.TEX_Skybox, 0u);
    context.bind_program(PROG_Lighting_Skybox);

    context.bind_vertexarray_dummy();
    context.draw_arrays(sq::DrawPrimitive::TriangleStrip, 0u, 4u);

    //-- Render Opaque Pass ----------------------------------//

    context.bind_framebuffer(FB_MsMain);

    for (const auto& object : mRenderObjects)
        object->render_opaque();

    //-- Render Transparent Pass -----------------------------//

    context.bind_framebuffer(FB_MsMain);

    for (const auto& object : mRenderObjects)
        object->render_transparent();
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

    FB_MsMain.blit(FB_Resolve, options.window_size, sq::BlitMask::DepthColour);
}

//============================================================================//

void Renderer::finish_rendering()
{
    //-- Composite to the Default Framebuffer ----------------//

    context.bind_framebuffer_default();

    context.set_state(sq::BlendMode::Disable);
    context.set_state(sq::CullFace::Disable);
    context.set_state(sq::DepthTest::Disable);

    context.bind_texture(TEX_Colour, 0u);
    context.bind_program(PROG_Composite);

    context.bind_vertexarray_dummy();
    context.draw_arrays(sq::DrawPrimitive::TriangleStrip, 0u, 4u);
}
