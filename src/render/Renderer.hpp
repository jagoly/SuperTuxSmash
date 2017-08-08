#pragma once

#include <sqee/app/PreProcessor.hpp>

#include <sqee/gl/FrameBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>

#include <sqee/render/Mesh.hpp>

#include "main/Options.hpp"

//====== Forward Declarations ================================================//

namespace sts { struct HitBlob; struct HurtBlob; class RenderObject; }

//============================================================================//

namespace sts {

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const Options& options);

    void refresh_options();

    //--------------------------------------------------------//

    void add_object(unique_ptr<RenderObject> object);

    //--------------------------------------------------------//

    void set_camera_view_bounds(Vec2F min, Vec2F max);

    void render_objects(float accum, float blend);

    void render_blobs(const std::vector<HitBlob*>& blobs);

    void render_blobs(const std::vector<HurtBlob*>& blobs);

    void finish_rendering();

    //--------------------------------------------------------//

    struct {

        sq::FrameBuffer Depth;
        sq::FrameBuffer Main;
        sq::FrameBuffer Resolve;
        //sq::FrameBuffer Final;

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

    struct { sq::Mesh Sphere; sq::Mesh Capsule; } meshes;

    //--------------------------------------------------------//

    struct { sq::UniformBuffer ubo; Mat4F viewMatrix, projMatrix; } camera;

    struct { sq::UniformBuffer ubo; } light;

    //--------------------------------------------------------//

    sq::PreProcessor processor;

    sq::Context& context;

private: //===================================================//

    Vec2F mPreviousViewBoundsMin;
    Vec2F mPreviousViewBoundsMax;

    Vec2F mCurrentViewBoundsMin;
    Vec2F mCurrentViewBoundsMax;

    //--------------------------------------------------------//

    std::vector<unique_ptr<RenderObject>> mRenderObjects;

    //--------------------------------------------------------//

    const Options& options;
};

} // namespace sts
