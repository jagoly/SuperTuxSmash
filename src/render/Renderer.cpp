#include "render/Renderer.hpp"

#include "main/Options.hpp"

#include "render/Camera.hpp"
#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"

#include <sqee/app/PreProcessor.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/gl/Context.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>

using namespace sts;

//============================================================================//

Renderer::Renderer(const Options& options, sq::PreProcessor& processor, ResourceCaches& caches)
    : context(sq::Context::get()), options(options), processor(processor), caches(caches)
{
    mDebugRenderer = std::make_unique<DebugRenderer>(*this);
    mParticleRenderer = std::make_unique<ParticleRenderer>(*this);

    mLightUbo.allocate_dynamic(112u);
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

//============================================================================//

int64_t Renderer::create_draw_items(const std::vector<DrawItemDef>& defs, const sq::FixedBuffer* ubo,
                                    std::map<TinyString, const bool*> conditions)
{
    const int64_t groupId = ++mCurrentGroupId;

    for (const DrawItemDef& def : defs)
    {
        DrawItem& item = mDrawItems.emplace_back();

        if (def.condition.empty() == false)
            item.condition = conditions.at(def.condition);
        else item.condition = nullptr;

        item.material = def.material;
        item.mesh = def.mesh;

        item.pass = def.pass;
        item.invertCondition = def.invertCondition;
        item.subMesh = def.subMesh;

        item.ubo = ubo;
        item.groupId = groupId;
    }

    // todo: sort draw items here

    return groupId;
}

void Renderer::delete_draw_items(int64_t groupId)
{
    const auto predicate = [groupId](DrawItem& item) { return item.groupId == groupId; };
    algo::erase_if(mDrawItems, predicate);
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

    //--------------------------------------------------------//

    const auto render_items_for_draw_pass = [&](DrawPass pass)
    {
        const sq::Material* prevMtrl = nullptr;
        const sq::FixedBuffer* prevUbo = nullptr;
        const sq::Mesh* prevMesh = nullptr;

        for (const DrawItem& item : mDrawItems)
        {
            // skip item if wrong pass or condition not satisfied
            if (item.pass != pass || (item.condition && *item.condition == item.invertCondition))
                continue;

            if (prevMtrl != &item.material.get())
                item.material->apply_to_context(context),
                    prevMtrl = &item.material.get();

            if (prevUbo != item.ubo)
                context.bind_buffer(*item.ubo, sq::BufTarget::Uniform, 2u),
                    prevUbo = item.ubo;

            if (prevMesh != &item.mesh.get())
                item.mesh->apply_to_context(context);
                    prevMesh = &item.mesh.get();

            if (item.subMesh < 0) item.mesh->draw_complete(context);
            else item.mesh->draw_submesh(context, uint(item.subMesh));
        }
    };

    //-- Render Opaque Pass ----------------------------------//

    context.bind_framebuffer(FB_MsMain);

    context.set_state(sq::DepthTest::Replace);
    context.set_depth_compare(sq::CompareFunc::LessEqual);

    render_items_for_draw_pass(DrawPass::Opaque);

    //-- Render Transparent Pass -----------------------------//

    context.bind_framebuffer(FB_MsMain);

    context.set_state(sq::DepthTest::Keep);
    context.set_state(sq::BlendMode::Alpha);
    context.set_state(sq::CullFace::Disable);
    context.set_depth_compare(sq::CompareFunc::LessEqual);

    render_items_for_draw_pass(DrawPass::Transparent);
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
