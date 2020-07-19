#include "render/DebugRender.hpp"

#include "game/Blobs.hpp"
#include "game/Fighter.hpp"
#include "game/FightWorld.hpp"
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
    mDiamondMesh.load_from_file("assets/debug/Diamond.sqm", true);

    //-- Set up the vao and vbo ------------------------------//

    mLineVertexArray.set_vertex_buffer(mLineVertexBuffer, sizeof(Vec4F));
    mLineVertexArray.add_float_attribute(0u, 4u, gl::FLOAT, false, 0u);

    // allocate space for up to 128 lines
    mLineVertexBuffer.allocate_dynamic(sizeof(Vec4F) * 256u, nullptr);
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
    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mSphereMesh.get_vao());

    mBlobShader.update(2, 0.25f);

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

        const float angleA = maths::radians(blob->knockAngle * float(blob->fighter->status.facing));
        const float angleB = maths::radians((blob->knockAngle + 0.0625f) * float(blob->fighter->status.facing));
        const float angleC = maths::radians((blob->knockAngle - 0.0625f) * float(blob->fighter->status.facing));
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
    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mCapsuleMesh.get_vao());

    mBlobShader.update(2, 0.25f);

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

//============================================================================//

void DebugRenderer::render_diamond(const Fighter& fighter)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_FrameBuffer(renderer.fbos.Resolve);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mDiamondMesh.get_vao());

    mBlobShader.update(2, 0.25f);

    //--------------------------------------------------------//

    const Vec3F translate = Vec3F(fighter.get_position() + Vec2F(0.f, fighter.get_diamond().offsetCross), 0.f);
    const float scaleBottom = fighter.get_diamond().offsetCross;
    const float scaleTop = fighter.get_diamond().offsetTop - fighter.get_diamond().offsetCross;

    const Mat4F bottomMat = maths::transform(translate, Vec3F(fighter.get_diamond().halfWidth, scaleBottom, 0.1f));
    const Mat4F topMat = maths::transform(translate, Vec3F(fighter.get_diamond().halfWidth, scaleTop, 0.1f));

    mBlobShader.update(1, Vec3F(1.f, 1.f, 1.f));

    mBlobShader.update(0, projViewMat * bottomMat);
    mDiamondMesh.draw_partial(0u);

    mBlobShader.update(0, projViewMat * topMat);
    mDiamondMesh.draw_partial(1u);
}

//============================================================================//

void DebugRenderer::render_skeleton(const Fighter& fighter)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_FrameBuffer(renderer.fbos.Resolve);

    context.set_state(Context::Blend_Mode::Alpha);
    context.set_state(Context::Cull_Face::Back);
    context.set_state(Context::Depth_Test::Disable);

    context.bind_Program(mBlobShader);

    context.bind_VertexArray(mSphereMesh.get_vao());

    mBlobShader.update(1, Vec3F(1.f, 1.f, 1.f));
    mBlobShader.update(2, 0.5f);

    //--------------------------------------------------------//

    const Vector<Mat4F> mats = fighter.debug_get_skeleton_mats();

    Vector<Vec4F> lineData;
    lineData.reserve(mats.size() - 1u);

    const Mat4F projViewModelMat = projViewMat * fighter.get_model_matrix();

    //--------------------------------------------------------//

    for (uint i = 0u; i < mats.size(); ++i)
    {
        mBlobShader.update(0, projViewModelMat * maths::scale(mats[i], Vec3F(0.02f)));
        mSphereMesh.draw_complete();

        if (int8_t parent = fighter.get_armature().get_bone_parent(i); parent != -1)
        {
            const Vec4F pA = (projViewModelMat * mats[parent])[3];
            const Vec4F pB = (projViewModelMat * mats[i])[3];

            lineData.insert(lineData.end(), {pA, pB});
        }
    }

    //--------------------------------------------------------//

    context.set_state(Context::Blend_Mode::Disable);

    gl::LineWidth(2.f);

    context.bind_Program(mArrowShader);

    context.bind_VertexArray(mLineVertexArray);

    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * lineData.size()), lineData.data());

    gl::DrawArrays(gl::LINES, 0, int(lineData.size()));
}
