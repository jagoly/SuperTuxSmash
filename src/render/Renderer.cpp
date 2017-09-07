#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Misc.hpp>

#include <sqee/maths/Functions.hpp>

#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>

#include "game/Blobs.hpp"

#include "render/RenderObject.hpp"

#include "render/Renderer.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Renderer::Renderer(const Options& options) : context(sq::Context::get()), options(options)
{
    //-- Load Mesh Objects -----------------------------------//

    meshes.Sphere.load_from_file("debug/Sphere", true);
    meshes.Capsule.load_from_file("debug/Capsule", true);

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

    camera.ubo.create_and_allocate(288u);
    light.ubo.create_and_allocate(112u);
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

    processor.load_vertex(shaders.Lighting_Skybox, "lighting/Skybox_vs");
    processor.load_vertex(shaders.Debug_HitBlob, "debug/HitBlob_vs");
    processor.load_vertex(shaders.Composite, "FullScreen_vs");

    processor.load_fragment(shaders.Lighting_Skybox, "lighting/Skybox_fs");
    processor.load_fragment(shaders.Debug_HitBlob, "debug/HitBlob_fs");
    processor.load_fragment(shaders.Composite, "Composite_fs");

    //-- Link Shader Program Stages --------------------------//

    shaders.Depth_SimpleSolid.link_program_stages();
    shaders.Depth_SkellySolid.link_program_stages();
    shaders.Depth_SimplePunch.link_program_stages();
    shaders.Depth_SkellyPunch.link_program_stages();

    shaders.Lighting_Skybox.link_program_stages();
    shaders.Debug_HitBlob.link_program_stages();
    shaders.Composite.link_program_stages();
}

//============================================================================//

void Renderer::add_object(unique_ptr<RenderObject> object)
{
    mRenderObjects.push_back(std::move(object));
}

void Renderer::set_camera_view_bounds(Vec2F min, Vec2F max)
{
    mPreviousBounds = mCurrentBounds;

    mCurrentBounds.origin = Vec3F((min + max) * 0.5f, 0.f);
    mCurrentBounds.radius = maths::distance(min, max) * 0.5f;
}

void Renderer::set_camera_view_bounds(sq::maths::Sphere bounds)
{
    mPreviousBounds = mCurrentBounds;
    mCurrentBounds = bounds;
}

//============================================================================//

struct StaticShit
{
    StaticShit()
    {
        TEX_Skybox.set_filter_mode(true);
        TEX_Skybox.set_mipmaps_mode(true);

        TEX_Skybox.allocate_storage(2048u);
        TEX_Skybox.load_directory("skybox");
        TEX_Skybox.generate_auto_mipmaps();
    }

    sq::TextureCube TEX_Skybox { sq::Texture::Format::RGB8_UN };
};

//============================================================================//

void Renderer::render_objects(float elapsed, float blend)
{
    static StaticShit shit;

    //-- Camera and Light can spin, for funzies --------------//

    //static Vec3F cameraPosition = { 0.f, +3.f, -6.f };
    static Vec3F skyDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));

    #ifdef SQEE_DEBUG
    //if (sqeeDebugToggle1) cameraPosition = maths::rotate_y(cameraPosition, 0.1f * elapsed);
    if (sqeeDebugToggle2) skyDirection = maths::rotate_y(skyDirection, 0.1f * elapsed);
    #endif

    //-- Update the Camera -----------------------------------//

    const Vec3F blendOrigin = maths::mix(mPreviousBounds.origin, mCurrentBounds.origin, blend);
    const float blendRadius = maths::mix(mPreviousBounds.radius, mCurrentBounds.radius, blend);

    const Vec3F cameraTarget = blendOrigin + Vec3F(0.f, 1.f, 0.f);
    const float cameraDistance = maths::max((blendRadius + 0.5f) / std::tan(0.5f), 4.f);

    const Vec3F cameraDirection = maths::normalize(Vec3F(0.f, -1.f, +3.f));
    const Vec3F cameraPosition = cameraTarget - cameraDirection * cameraDistance;


    const Vec2F viewSize = Vec2F(options.Window_Size);

    camera.viewMatrix = maths::look_at_LH(cameraPosition, cameraTarget, Vec3F(0.f, 1.f, 0.f));
    camera.projMatrix = maths::perspective_LH(1.f, viewSize.x / viewSize.y, 0.2f, 200.f);

    const Mat4F inverseViewMat = maths::inverse(camera.viewMatrix);
    const Mat4F inverseProjMat = maths::inverse(camera.projMatrix);

    camera.ubo.update_complete ( camera.viewMatrix, camera.projMatrix,
                                 inverseViewMat, inverseProjMat,
                                 cameraPosition, 0, cameraDirection, 0 );


    //-- Update the Lighting ---------------------------------//

    const Vec3F ambiColour = { 0.5f, 0.5f, 0.5f };
    const Vec3F skyColour = { 0.5f, 0.5f, 0.5f };
    const Mat4F skyMatrix = Mat4F();

    light.ubo.update_complete(ambiColour, 0, skyColour, 0, skyDirection, 0, skyMatrix);

    //-- Integrate Object Changes ----------------------------//

    for (const auto& object : mRenderObjects)
        object->integrate(blend);

    //-- Setup Shared Rendering State ------------------------//

    context.set_ViewPort(options.Window_Size);

    context.bind_UniformBuffer(camera.ubo, 0u);
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
    context.bind_FrameBuffer(fbos.Main);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(shaders.Debug_HitBlob);

    context.bind_VertexArray(meshes.Sphere.get_vao());

    const Mat4F projViewMat = camera.projMatrix * camera.viewMatrix;

    for (const HitBlob* blob : blobs)
    {
        const maths::Sphere& s = blob->sphere;

        const Mat4F matrix = maths::transform(s.origin, Vec3F(s.radius));

        shaders.Debug_HitBlob.update(0, projViewMat * matrix);
        shaders.Debug_HitBlob.update(1, blob->get_debug_colour());

        meshes.Sphere.draw_complete();
    }
}

//============================================================================//

void Renderer::render_blobs(const std::vector<HurtBlob*>& blobs)
{
    context.bind_FrameBuffer(fbos.Main);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(shaders.Debug_HitBlob);

    context.bind_VertexArray(meshes.Capsule.get_vao());

    const Mat4F projViewMat = camera.projMatrix * camera.viewMatrix;

    for (const HurtBlob* blob : blobs)
    {
        const maths::Capsule& c = blob->capsule;

        const Mat3F rotation = maths::basis_from_y(maths::normalize(c.originB - c.originA));

        const Vec3F originM = (c.originA + c.originB) * 0.5f;
        const float lengthM = maths::distance(c.originA, c.originB);

        const Mat4F matrixA = maths::transform(c.originA, rotation, c.radius);
        const Mat4F matrixB = maths::transform(c.originB, rotation, c.radius);

        const Mat4F matrixM = maths::transform(originM, rotation, Vec3F(c.radius, lengthM, c.radius));

        shaders.Debug_HitBlob.update(1, blob->get_debug_colour());

        shaders.Debug_HitBlob.update(0, projViewMat * matrixA);
        meshes.Capsule.draw_partial(0u);

        shaders.Debug_HitBlob.update(0, projViewMat * matrixB);
        meshes.Capsule.draw_partial(1u);

        shaders.Debug_HitBlob.update(0, projViewMat * matrixM);
        meshes.Capsule.draw_partial(2u);
    }
}
