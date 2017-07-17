#pragma once

#include <sqee/app/PreProcessor.hpp>

#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

#include <sqee/render/Volume.hpp>

#include "game/HitBlob.hpp"

#include "render/RenderEntity.hpp"

#include "main/Options.hpp"

//====== Data Declarations ===================================================//

extern "C" const float data_CubeVertices [8*3];
extern "C" const uchar data_CubeIndices  [12*3];
extern "C" const float data_SphereVertices [42*3];
extern "C" const uchar data_SphereIndices  [80*3];

//============================================================================//

namespace sts {

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const Options& options);

    void refresh_options();

    //--------------------------------------------------------//

    void add_entity(unique_ptr<RenderEntity> entity);

    //--------------------------------------------------------//

    void render(float accum, float blend);

    void render_hit_blobs(const std::vector<HitBlob*>& blobs);

    //--------------------------------------------------------//

    struct {

        sq::FrameBuffer Depth;
        sq::FrameBuffer Main;
        sq::FrameBuffer Resolve;
        sq::FrameBuffer Final;

    } fbos;

    //--------------------------------------------------------//

    struct {

        sq::TextureMulti Depth { sq::Texture::Format::DEP24S8 };
        sq::TextureMulti Colour { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Resolve { sq::Texture::Format::RGB16_FP };
        sq::Texture2D Final { sq::Texture::Format::RGBA8_UN };

    } textures;

    //--------------------------------------------------------//

    struct {

        sq::Program Depth_SimpleSolid;
        sq::Program Depth_SkellySolid;
        sq::Program Depth_SimplePunch;
        sq::Program Depth_SkellyPunch;

        sq::Program Lighting_Skybox;
        sq::Program Debug_HitBlob;
        sq::Program Composite;

    } shaders;

    //--------------------------------------------------------//

    struct {

        sq::Volume Cube { data_CubeVertices, data_CubeIndices, 8u, 36u };
        sq::Volume Sphere { data_SphereVertices, data_SphereIndices, 42u, 240u };

    } volumes;

    //--------------------------------------------------------//

    struct {

        sq::UniformBuffer ubo;
        Mat4F viewMatrix, projMatrix;

    } camera;

    struct {

        sq::UniformBuffer ubo;

    } light;

    //--------------------------------------------------------//

    sq::PreProcessor processor;

    sq::Context& context;

private: //===================================================//

    std::vector<unique_ptr<RenderEntity>> entities;

    const Options& options;
};

} // namespace sts
