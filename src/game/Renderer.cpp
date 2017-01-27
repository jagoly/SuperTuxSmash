#include <sqee/misc/StringCast.hpp>

#include <sqee/gl/Context.hpp>
#include <sqee/gl/Drawing.hpp>

#include <main/Options.hpp>

#include "Renderer.hpp"

using namespace sts;
using Context = sq::Context;
namespace maths = sq::maths;

//============================================================================//

Renderer::~Renderer() = default;

//============================================================================//

Renderer::Renderer()
{
    // Set Texture Paramaters /////

    textures.Main.set_filter_mode(true);
    textures.Final.set_filter_mode(true);

    // Import GLSL Headers /////

    shaders.preprocs.import_header("headers/blocks/Camera");
    shaders.preprocs.import_header("headers/blocks/Light");

    shaders.preprocs.import_header("headers/super/Model_vs");
    shaders.preprocs.import_header("headers/super/Model_fs");

    // Create Uniform Buffers /////

    camera.ubo.reserve("view_mat", 16); // Mat4F
    camera.ubo.reserve("proj_mat", 16); // Mat4F
    camera.ubo.reserve("view_inv", 16); // Mat4F
    camera.ubo.reserve("proj_inv", 16); // Mat4F
    camera.ubo.reserve("pos", 4); // Vec3F
    camera.ubo.reserve("dir", 4); // Vec3F
    camera.ubo.create_and_allocate();

    light.ubo.reserve("ambi_colour", 4); // Vec3F
    light.ubo.reserve("sky_colour", 4); // Vec3F
    light.ubo.reserve("sky_direction", 4); // Vec3F
    light.ubo.reserve("sky_matrix", 16); // Mat4F
    light.ubo.create_and_allocate();
}

//============================================================================//

void Renderer::update_options()
{
    static const auto& options = Options::get();

    //========================================================//

    string headerStr = "// set of constants and defines added at runtime\n";

    headerStr += "const uint OPTION_WinWidth  = " + std::to_string(options.Window_Size.x) + ";\n";
    headerStr += "const uint OPTION_WinHeight = " + std::to_string(options.Window_Size.y) + ";\n";

    if (options.Bloom_Enable == true) headerStr += "#define OPTION_BLOOM_ENABLE\n";;
    if (options.SSAO_Quality != 0u)   headerStr += "#define OPTION_SSAO_ENABLE\n";
    if (options.FSAA_Quality != 0u)   headerStr += "#define OPTION_FSAA_ENABLE\n";
    if (options.SSAO_Quality >= 2u)   headerStr += "#define OPTION_SSAO_HIGH\n";
    if (options.FSAA_Quality >= 2u)   headerStr += "#define OPTION_FSAA_HIGH\n";

    headerStr += "// some handy shortcuts for comman use of this data\n"
                 "const float OPTION_Aspect = float(OPTION_WinWidth) / float(OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeFull = vec2(OPTION_WinWidth, OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeHalf = round(OPTION_WinSizeFull / 2.f);\n"
                 "const vec2 OPTION_WinSizeQter = round(OPTION_WinSizeFull / 4.f);\n";

    shaders.preprocs.update_header("runtime/Options", headerStr);

    //========================================================//

    textures.Depth.allocate_storage(options.Window_Size);
    textures.Main.allocate_storage(options.Window_Size);
    textures.Final.allocate_storage(options.Window_Size);

    fbos.Depth.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);
    fbos.Main.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);
    fbos.Final.attach(gl::DEPTH_STENCIL_ATTACHMENT, textures.Depth);
    fbos.Main.attach(gl::COLOR_ATTACHMENT0, textures.Main);
    fbos.Final.attach(gl::COLOR_ATTACHMENT0, textures.Final);

    //========================================================//

    shaders.preprocs(shaders.VS_FullScreen, "generic/FullScreen_vs");
    shaders.preprocs(shaders.FS_PassThrough, "generic/PassThrough_fs");

    shaders.preprocs(shaders.VS_Lighting_Skybox, "lighting/Skybox_vs");
    shaders.preprocs(shaders.FS_Lighting_Skybox, "lighting/Skybox_fs");

    shaders.preprocs(shaders.FS_Composite, "composite/Composite_fs");
    shaders.preprocs(shaders.FS_FSAA_Screen, "composite/FSAA/FXAA_fs");
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
    }

    sq::TextureCube TEX_Skybox { sq::Texture::Format::RGB8_UN };
};

//============================================================================//

void Renderer::render()
{
    static auto& context = Context::get();
    static const auto& options = Options::get();

    //========================================================//

    static StaticShit shit;

    //========================================================//

    static Vec3F cameraPosition = { 0.f, -10.f, +5.f };
    cameraPosition = maths::rotate_z(cameraPosition, 0.0005f);

    const Vec3F cameraDirection = maths::normalize(-cameraPosition);

    const Vec2F size = Vec2F(options.Window_Size);

    camera.viewMatrix = maths::look_at(cameraPosition, Vec3F(), Vec3F(0.f, 0.f, 1.f));
    camera.projMatrix = maths::perspective(1.f, size.x / size.y, 0.2f, 200.f);

    const Mat4F inverseViewMat = maths::inverse(camera.viewMatrix);
    const Mat4F inverseProjMat = maths::inverse(camera.projMatrix);

    camera.ubo.update("view_mat", &camera.viewMatrix);
    camera.ubo.update("proj_mat", &camera.projMatrix);
    camera.ubo.update("view_inv", &inverseViewMat);
    camera.ubo.update("proj_inv", &inverseProjMat);
    camera.ubo.update("pos", &cameraPosition);
    camera.ubo.update("dir", &cameraDirection);

    //========================================================//

    Vec3F ambiColour = { 0.4f, 0.4f, 0.4f };
    Vec3F skyColour = { 0.9f, 0.9f, 0.9f };
    //Vec3F skyDirection = { -0.35396f, +0.46348f, -0.81234f };
    Vec3F skyDirection = { 0.f, 0.5f, -1.f };
    Mat4F skyMatrix = Mat4F();

    light.ubo.update("ambi_colour", &ambiColour);
    light.ubo.update("sky_colour", &skyColour);
    light.ubo.update("sky_direction", &skyDirection);
    light.ubo.update("sky_matrix", &skyMatrix);

    //========================================================//

    context.bind_shader_pipeline();

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

    context.use_Shader_Vert(shaders.VS_Lighting_Skybox);
    context.use_Shader_Frag(shaders.FS_Lighting_Skybox);

    context.bind_Texture(shit.TEX_Skybox, 0u);

    sq::draw_screen_quad();

    //========================================================//

    if (functions.draw_FighterA) functions.draw_FighterA();
    if (functions.draw_FighterB) functions.draw_FighterB();

    //========================================================//

    context.set_state(Context::Blend_Mode::Disable);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_FrameBuffer_default();

    context.use_Shader_Vert(shaders.VS_FullScreen);
    context.use_Shader_Frag(shaders.FS_Composite);

    context.bind_Texture(textures.Main, 0u);

    sq::draw_screen_quad();
}
