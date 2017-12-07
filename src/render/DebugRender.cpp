#include <sqee/maths/Functions.hpp>
#include <sqee/gl/Context.hpp>

#include "game/Blobs.hpp"

#include "render/Camera.hpp"
#include "render/DebugRender.hpp"

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

DebugRender::DebugRender(Renderer& renderer) : renderer(renderer)
{
    //-- Load Mesh Objects -----------------------------------//

    mSphereMesh.load_from_file("debug/Sphere", true);
    mCapsuleMesh.load_from_file("debug/Capsule", true);
}

//============================================================================//

void DebugRender::refresh_options()
{
    const auto& processor = renderer.processor;

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(mBlobShader, "debug/HitBlob_vs");
    processor.load_fragment(mBlobShader, "debug/HitBlob_fs");

    //-- Link Shader Program Stages --------------------------//

    mBlobShader.link_program_stages();
}

//============================================================================//

void DebugRender::render_blobs(const std::vector<HitBlob*>& blobs)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_FrameBuffer(renderer.fbos.Main);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mSphereMesh.get_vao());

    //--------------------------------------------------------//

    for (const HitBlob* blob : blobs)
    {
        const maths::Sphere& s = blob->sphere;

        const Mat4F matrix = maths::transform(s.origin, Vec3F(s.radius));

        mBlobShader.update(0, projViewMat * matrix);
        mBlobShader.update(1, blob->get_debug_colour());

        mSphereMesh.draw_complete();
    }
}

//============================================================================//

void DebugRender::render_blobs(const std::vector<HurtBlob*>& blobs)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_FrameBuffer(renderer.fbos.Main);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mCapsuleMesh.get_vao());

    //--------------------------------------------------------//

    for (const HurtBlob* blob : blobs)
    {
        const maths::Capsule& c = blob->capsule;

        const Mat3F rotation = maths::basis_from_y(maths::normalize(c.originB - c.originA));

        const Vec3F originM = (c.originA + c.originB) * 0.5f;
        const float lengthM = maths::distance(c.originA, c.originB);

        const Mat4F matrixA = maths::transform(c.originA, rotation, c.radius);
        const Mat4F matrixB = maths::transform(c.originB, rotation, c.radius);

        const Mat4F matrixM = maths::transform(originM, rotation, Vec3F(c.radius, lengthM, c.radius));

        mBlobShader.update(1, blob->get_debug_colour());

        mBlobShader.update(0, projViewMat * matrixA);
        mCapsuleMesh.draw_partial(0u);

        mBlobShader.update(0, projViewMat * matrixB);
        mCapsuleMesh.draw_partial(1u);

        mBlobShader.update(0, projViewMat * matrixM);
        mCapsuleMesh.draw_partial(2u);
    }
}
