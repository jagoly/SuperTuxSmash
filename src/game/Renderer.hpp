#pragma once

#include <sqee/app/PreProcessor.hpp>

#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

#include <sqee/render/Mesh.hpp>
#include <sqee/render/Armature.hpp>
#include <sqee/render/Volume.hpp>

#include "main/Options.hpp"

//====== Data Declarations ===================================================//

extern "C" const float data_CubeVertices [8*3];
extern "C" const uchar data_CubeIndices  [12*3];
extern "C" const float data_SphereVertices [42*3];
extern "C" const uchar data_SphereIndices  [80*3];

//====== Forward Declarations ================================================//

namespace sts { class Game; }

//============================================================================//

namespace sts {

//============================================================================//

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(Game& game, const Options& options);

    //========================================================//

    void refresh_options();

    void render(float blend);

    //========================================================//

    struct {

        sq::FrameBuffer Depth;
        sq::FrameBuffer Main;
        sq::FrameBuffer Resolve;
        sq::FrameBuffer Final;

    } fbos;

    //========================================================//

    struct {

        sq::TextureMulti Depth { sq::Texture::Format::DEP24S8 };
        sq::TextureMulti Colour { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Resolve { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Final { sq::Texture::Format::RGBA8_UN };

    } textures;

    //========================================================//

    struct {

        sq::Program PROG_Depth_SimpleSolid;
        sq::Program PROG_Depth_SkellySolid;
        sq::Program PROG_Depth_SimplePunch;
        sq::Program PROG_Depth_SkellyPunch;

        sq::Program PROG_PassThrough;

        sq::Program PROG_Lighting_Skybox;

        sq::Program PROG_Composite;

        sq::Program PROG_Debug_HitShape;

    } shaders;

    //========================================================//

    struct {

        sq::Volume Cube { data_CubeVertices, data_CubeIndices, 8u, 36u };
        sq::Volume Sphere { data_SphereVertices, data_SphereIndices, 42u, 240u };

    } volumes;

    //========================================================//

    struct {

        sq::UniformBuffer ubo;
        Mat4F viewMatrix, projMatrix;

    } camera;

    struct {

        sq::UniformBuffer ubo;

    } light;

    //--------------------------------------------------------//

    sq::PreProcessor processor;

private: //===================================================//

    Game& game;

    const Options& options;
};

//============================================================================//

} // namespace client
