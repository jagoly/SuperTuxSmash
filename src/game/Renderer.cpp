#include <sqee/misc/StringCast.hpp>
#include <sqee/debug/Misc.hpp>

#include <sqee/maths/Functions.hpp>

#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>

#include <sqee/render/Mesh.hpp>

#include "game/Game.hpp"
#include "game/Fighter.hpp"

#include "Renderer.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Renderer::Renderer(Game& game, const Options& options) : game(game), options(options)
{
    // Set Texture Paramaters /////

    textures.Colour.set_filter_mode(true);
    textures.Final.set_filter_mode(true);

    // Import GLSL Headers /////

    processor.import_header("headers/blocks/Camera");
    processor.import_header("headers/blocks/Light");
    processor.import_header("headers/blocks/Skeleton");

    processor.import_header("headers/super/Simple_vs");
    processor.import_header("headers/super/Skelly_vs");
    processor.import_header("headers/super/Model_fs");

    // Create Uniform Buffers /////

    camera.ubo.create_and_allocate(284u);

    light.ubo.create_and_allocate(112u);
}

//============================================================================//

void Renderer::refresh_options()
{
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

    //========================================================//

    textures.Depth.allocate_storage(Vec3U(options.Window_Size, 4u));
    textures.Colour.allocate_storage(Vec3U(options.Window_Size, 4u));
    textures.Resolve.allocate_storage(options.Window_Size);
    textures.Final.allocate_storage(options.Window_Size);

    //========================================================//

    fbos.Depth.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);

    fbos.Main.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);
    fbos.Main.attach(gl::COLOR_ATTACHMENT0, textures.Colour);

    fbos.Resolve.attach(gl::COLOR_ATTACHMENT0, textures.Resolve);

    fbos.Final.attach(gl::COLOR_ATTACHMENT0, textures.Final);

    //========================================================//

    processor.load_vertex(shaders.PROG_Depth_SimpleSolid, "generic/depth/Simple_vs");
    shaders.PROG_Depth_SimpleSolid.link_program_stages();

    processor.load_vertex(shaders.PROG_Depth_SkellySolid, "generic/depth/Skelly_vs");
    shaders.PROG_Depth_SkellySolid.link_program_stages();

    processor.load_vertex(shaders.PROG_Depth_SimplePunch, "generic/depth/Simple_vs");
    processor.load_fragment(shaders.PROG_Depth_SimplePunch, "generic/depth/Mask_fs");
    shaders.PROG_Depth_SimplePunch.link_program_stages();

    processor.load_vertex(shaders.PROG_Depth_SkellyPunch, "generic/depth/Skelly_vs");
    processor.load_fragment(shaders.PROG_Depth_SkellyPunch, "generic/depth/Mask_fs");
    shaders.PROG_Depth_SkellyPunch.link_program_stages();

    processor.load_vertex(shaders.PROG_PassThrough, "generic/FullScreen_vs");
    processor.load_fragment(shaders.PROG_PassThrough, "generic/PassThrough_fs");
    shaders.PROG_PassThrough.link_program_stages();

    processor.load_vertex(shaders.PROG_Lighting_Skybox, "lighting/Skybox_vs");
    processor.load_fragment(shaders.PROG_Lighting_Skybox, "lighting/Skybox_fs");
    shaders.PROG_Lighting_Skybox.link_program_stages();

    processor.load_vertex(shaders.PROG_Composite, "generic/FullScreen_vs");
    processor.load_fragment(shaders.PROG_Composite, "composite/Composite_fs");
    shaders.PROG_Composite.link_program_stages();

    processor.load_vertex(shaders.PROG_Debug_HitShape, "debug/HitShape_vs");
    processor.load_fragment(shaders.PROG_Debug_HitShape, "debug/HitShape_fs");
    shaders.PROG_Debug_HitShape.link_program_stages();
}

//============================================================================//

struct StaticShit
{
    StaticShit()
    {
        TEX_Skybox.set_filter_mode(true);
        TEX_Skybox.set_mipmaps_mode(true);

        TEX_Skybox.allocate_storage(2048u);

        TEX_Skybox.load_file("skybox/0_right",   0u);
        TEX_Skybox.load_file("skybox/1_left",    1u);
        TEX_Skybox.load_file("skybox/2_forward", 2u);
        TEX_Skybox.load_file("skybox/3_back",    3u);
        TEX_Skybox.load_file("skybox/4_up",      4u);
        TEX_Skybox.load_file("skybox/5_down",    5u);

        TEX_Skybox.generate_auto_mipmaps();

        MESH_Sphere.load_from_file("debug/volumes/Sphere");
    }

    sq::TextureCube TEX_Skybox { sq::Texture::Format::RGB8_UN };

    sq::Mesh MESH_Sphere;
};

//============================================================================//

void Renderer::render(float blend)
{
    auto& context = Context::get();

    context.set_ViewPort(options.Window_Size);

    //========================================================//

    thread_local StaticShit shit;

    //========================================================//

    static Vec3F cameraPosition = { 0.f, -3.f, +1.5f };

    static Vec3F skyDirection = maths::normalize(Vec3F(0.f, 0.5f, -1.f));

    #ifdef SQEE_DEBUG
    if (sqeeDebugToggle1) cameraPosition = maths::rotate_z(cameraPosition, 0.0005f);
    if (sqeeDebugToggle2) skyDirection = maths::rotate_z(skyDirection, 0.0005f);
    #endif

    const Vec3F cameraDirection = maths::normalize(-cameraPosition);

    const Vec2F size = Vec2F(options.Window_Size);

    camera.viewMatrix = maths::look_at(cameraPosition, Vec3F(0.f, 0.f, 1.f), Vec3F(0.f, 0.f, 1.f));
    camera.projMatrix = maths::perspective(1.f, size.x / size.y, 0.2f, 200.f);

    const Mat4F inverseViewMat = maths::inverse(camera.viewMatrix);
    const Mat4F inverseProjMat = maths::inverse(camera.projMatrix);

    camera.ubo.update_complete ( camera.viewMatrix, camera.projMatrix, inverseViewMat,
                                 inverseProjMat, cameraPosition, 0, cameraDirection );

    //========================================================//

    const Vec3F ambiColour = { 0.5f, 0.5f, 0.5f };
    const Vec3F skyColour = { 0.5f, 0.5f, 0.5f };
    const Mat4F skyMatrix = Mat4F();

    light.ubo.update_complete(ambiColour, 0, skyColour, 0, skyDirection, 0, skyMatrix);

    //========================================================//

    context.bind_UniformBuffer(camera.ubo, 0u);
    context.bind_UniformBuffer(light.ubo, 1u);

    //========================================================//

    context.bind_FrameBuffer(fbos.Main);

    context.clear_Colour({0.f, 0.f, 0.f, 0.f});
    context.clear_Depth_Stencil();

    //========================================================//

    context.set_state(Context::Blend_Mode::Disable);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(shaders.PROG_Lighting_Skybox);

    context.bind_Texture(shit.TEX_Skybox, 0u);

    sq::draw_screen_quad();

    //========================================================//

    for (const auto& fighter : game.fighters)
        if (fighter != nullptr) fighter->integrate(blend);

    context.bind_FrameBuffer(fbos.Depth);
    for (const auto& fighter : game.fighters)
        if (fighter != nullptr) fighter->render_depth();

    context.bind_FrameBuffer(fbos.Main);
    for (const auto& fighter : game.fighters)
        if (fighter != nullptr) fighter->render_main();

    //========================================================//

    // debug stuff

//    context.set_state(Context::Blend_Mode::Alpha);
//    context.set_state(Context::Cull_Face::Disable);
//    context.set_state(Context::Depth_Test::Keep);
//    context.set_state(Context::Depth_Compare::Less);

//    context.use_Shader_Vert(shaders.VS_Debug_HitShape);
//    context.use_Shader_Frag(shaders.FS_Debug_HitShape);

//    for (const auto* hb : mGame.hitBlobs)
//    {
//        Mat4F matrix = Mat4F(Mat3F(hb->radius));
//        matrix[3] = Vec4F(hb->origin, 1.f);
//        matrix = camera.projMatrix * camera.viewMatrix * matrix;

//        shaders.VS_Debug_HitShape.update<Mat4F>("u_final_mat", matrix);
//        context.bind_VertexArray(shit.MESH_Sphere.get_vao());
//        shit.MESH_Sphere.draw_complete();
//    }

    //========================================================//

    fbos.Main.blit(fbos.Resolve, options.Window_Size, gl::COLOR_BUFFER_BIT);

    //========================================================//

    context.set_state(Context::Blend_Mode::Disable);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_FrameBuffer_default();

    context.bind_Program(shaders.PROG_Composite);

    context.bind_Texture(textures.Resolve, 0u);

    sq::draw_screen_quad();
}
