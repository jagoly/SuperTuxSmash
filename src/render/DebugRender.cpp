#include "render/DebugRender.hpp"

#include "game/Blobs.hpp"
#include "game/Fighter.hpp"
#include "render/Camera.hpp"

#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/redist/gl_loader.hpp>

#include <sqee/debug/Logging.hpp>

using Context = sq::Context;
namespace maths = sq::maths;
using namespace sts;

//============================================================================//

DebugRenderer::DebugRenderer(Renderer& renderer) : renderer(renderer)
{
    //-- Load Mesh Objects -----------------------------------//

    mSphereMesh.load_from_file("assets/debug/Sphere.sqm", true);
    mCapsuleMesh.load_from_file("assets/debug/Capsule.sqm", true);

    //-- Set up the vao and vbo ------------------------------//

    mLineVertexArray.set_vertex_buffer(mLineVertexBuffer, sizeof(Vec4F));
    mLineVertexArray.add_float_attribute(0u, 4u, gl::FLOAT, false, 0u);

    // allocate space for up to 32 lines
    mLineVertexBuffer.allocate_dynamic(sizeof(Vec4F) * 64u, nullptr);
}

//============================================================================//

void DebugRenderer::refresh_options()
{
    const auto& processor = renderer.processor;

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(mBlobShader, "debug/HitBlob_vs");
    processor.load_fragment(mBlobShader, "debug/HitBlob_fs");

    processor.load_vertex(mArrowShader, "debug/Arrow_vs");
    processor.load_fragment(mArrowShader, "debug/Arrow_fs");

    //-- Link Shader Program Stages --------------------------//

    mBlobShader.link_program_stages();
    mArrowShader.link_program_stages();
}

//============================================================================//

void DebugRenderer::render_hit_blobs(const Vector<HitBlob*>& blobs)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_FrameBuffer(renderer.fbos.Resolve);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Keep);
    context.set_state(Context::Depth_Compare::Less);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mSphereMesh.get_vao());

    //--------------------------------------------------------//

    Vector<Vec4F> lineData;
    lineData.reserve(blobs.size() * 2u);

    //--------------------------------------------------------//

    for (const HitBlob* blob : blobs)
    {
        const maths::Sphere& s = blob->sphere;

        const Mat4F matrix = maths::transform(s.origin, Vec3F(s.radius));

        mBlobShader.update(0, projViewMat * matrix);
        mBlobShader.update(1, blob->get_debug_colour());

        mSphereMesh.draw_complete();

        const float angleA = maths::radians(blob->knockAngle * float(blob->fighter->current.facing));
        const float angleB = maths::radians((blob->knockAngle + 0.0625f) * float(blob->fighter->current.facing));
        const float angleC = maths::radians((blob->knockAngle - 0.0625f) * float(blob->fighter->current.facing));
        const Vec3F offsetA = Vec3F(std::sin(angleA), std::cos(angleA), 0.f) * s.radius;
        const Vec3F offsetB = Vec3F(std::sin(angleB), std::cos(angleB), 0.f) * s.radius * 0.75f;
        const Vec3F offsetC = Vec3F(std::sin(angleC), std::cos(angleC), 0.f) * s.radius * 0.75f;

        const Vec4F pA = projViewMat * Vec4F(s.origin, 1.f);
        const Vec4F pB = projViewMat * Vec4F(s.origin + offsetA, 1.f);
        const Vec4F pC = projViewMat * Vec4F(s.origin + offsetB, 1.f);
        const Vec4F pD = projViewMat * Vec4F(s.origin + offsetC, 1.f);

        lineData.insert(lineData.end(), { pA,pB, pB,pC, pB,pD });
    }

    //--------------------------------------------------------//

    context.set_state(Context::Blend_Mode::Disable);

    gl::LineWidth(4.f);

    context.bind_Program(mArrowShader);

    context.bind_VertexArray(mLineVertexArray);

    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * lineData.size()), lineData.data());

    gl::DrawArrays(gl::LINES, 0, int(lineData.size()));
}

//============================================================================//

void DebugRenderer::render_hurt_blobs(const Vector<HurtBlob*>& blobs)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_FrameBuffer(renderer.fbos.Resolve);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Disable);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mCapsuleMesh.get_vao());

    //--------------------------------------------------------//

    for (const HurtBlob* blob : blobs)
    {
        const maths::Capsule& c = blob->capsule;

        const Vec3F difference = c.originB - c.originA;
        const float length = maths::length(difference);
        const Mat3F rotation = length > 0.00001f ? maths::basis_from_y(difference / length) : Mat3F();

        const Mat4F matrixA = maths::transform(c.originA, rotation, c.radius);
        const Mat4F matrixB = maths::transform(c.originB, rotation, c.radius);

        mBlobShader.update(1, blob->get_debug_colour());

        mBlobShader.update(0, projViewMat * matrixA);
        mCapsuleMesh.draw_partial(0u);

        mBlobShader.update(0, projViewMat * matrixB);
        mCapsuleMesh.draw_partial(1u);

        if (length > 0.00001f)
        {
            const Vec3F originM = (c.originA + c.originB) * 0.5f;
            const Mat4F matrixM = maths::transform(originM, rotation, Vec3F(c.radius, length, c.radius));

            mBlobShader.update(0, projViewMat * matrixM);
            mCapsuleMesh.draw_partial(2u);
        }
    }
}
