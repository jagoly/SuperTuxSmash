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

namespace sts {

//============================================================================//

// Forward Declarations /////

//class RenderStage;
class RenderFighter;

//============================================================================//

class Renderer final : sq::NonCopyable
{
public:

    //========================================================//

    Renderer();

    ~Renderer();

    //========================================================//

    //void set_stage(unique_ptr<RenderStage>&& stage);
    void add_fighter(unique_ptr<RenderFighter>&& fighter);

    //========================================================//

    void update_options();

    void render(float progress);

    //========================================================//

    struct {

        sq::FrameBuffer Depth;
        sq::FrameBuffer Main;
        sq::FrameBuffer Final;

    } fbos;

    //========================================================//

    struct {

        sq::Texture2D Depth { sq::Texture::Format::DEP24S8 };
        sq::Texture2D Main { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Final { sq::Texture::Format::RGBA8_UN };

    } textures;

    //========================================================//

    struct {

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

private:

    //========================================================//

    //unique_ptr<RenderStage> mStage;
    vector<unique_ptr<RenderFighter>> mFighters;
};

//============================================================================//

} // namespace sts
