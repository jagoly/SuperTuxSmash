#pragma once

#include <sqee/builtins.hpp>
#include <sqee/dop/Classes.hpp>

#include <sqee/maths/Vectors.hpp>
#include <sqee/maths/Matrices.hpp>

#include <sqee/app/PreProcessor.hpp>

#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Shaders.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

#include <sqee/render/Armature.hpp>
#include <sqee/render/Mesh.hpp>

namespace sts {

//============================================================================//

class Game; // Forward Declaration

//============================================================================//

class Renderer final : sq::NonCopyable
{
public:

    //========================================================//

    Renderer(Game& game);

    ~Renderer();

    //========================================================//

    float progress = 0.f;

    //========================================================//

    void update_options();

    void render();

    //========================================================//

    Game& mGame;

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

        sq::Shader VS_Depth_Simple { sq::Shader::Stage::Vertex };
        sq::Shader VS_Depth_Skelly { sq::Shader::Stage::Vertex };
        sq::Shader FS_Depth_Mask { sq::Shader::Stage::Fragment };

        sq::Shader VS_FullScreen { sq::Shader::Stage::Vertex };
        sq::Shader FS_PassThrough { sq::Shader::Stage::Fragment };

        sq::Shader VS_Lighting_Skybox { sq::Shader::Stage::Vertex };
        sq::Shader FS_Lighting_Skybox { sq::Shader::Stage::Fragment };

        sq::Shader FS_Composite { sq::Shader::Stage::Fragment };
        sq::Shader FS_FSAA_Screen { sq::Shader::Stage::Fragment };

        sq::PreProcessor preprocs;

    } shaders;

    //========================================================//

    struct {

        sq::UniformBuffer ubo;
        Mat4F viewMatrix, projMatrix;

    } camera;

    struct {

        sq::UniformBuffer ubo;

    } light;
};

//============================================================================//

} // namespace sts
