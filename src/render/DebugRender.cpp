#include "render/DebugRender.hpp"

#include "game/Action.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

#include <sqee/app/PreProcessor.hpp>
#include <sqee/gl/Context.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/objects/Armature.hpp>

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
    mLineVertexBuffer.allocate_dynamic(sizeof(Vec4F) * 256u);
}

//============================================================================//

void DebugRenderer::refresh_options()
{
    const auto& processor = renderer.processor;

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(mBlobShader, "shaders/debug/Blob_vs.glsl", {});
    processor.load_fragment(mBlobShader, "shaders/debug/Blob_fs.glsl", {});

    processor.load_vertex(mLinesShader, "shaders/debug/Lines_vs.glsl", {});
    processor.load_fragment(mLinesShader, "shaders/debug/Lines_fs.glsl", {});

    //-- Link Shader Program Stages --------------------------//

    mBlobShader.link_program_stages();
    mLinesShader.link_program_stages();
}

//============================================================================//

void DebugRenderer::render_hit_blobs(const std::vector<HitBlob*>& blobs)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_framebuffer(renderer.FB_Resolve);

    context.set_state(sq::BlendMode::Alpha);
    context.set_state(sq::CullFace::Back);
    context.set_state(sq::DepthTest::Disable);

    context.bind_program(mBlobShader);

    mSphereMesh.apply_to_context(context);

    mBlobShader.update(2, 0.25f);

    //--------------------------------------------------------//

    std::vector<Vec4F> lineData;
    lineData.reserve(blobs.size() * 3u);

    //--------------------------------------------------------//

    for (const HitBlob* blob : blobs)
    {
        const Mat4F matrix = maths::transform(blob->sphere.origin, Vec3F(blob->sphere.radius));

        mBlobShader.update(0, projViewMat * matrix);
        mBlobShader.update(1, blob->get_debug_colour());

        mSphereMesh.draw_complete(context);

        // for relative facing, we just show an arrow in the most likely direction
        // can give weird results when origin is close to zero
        const float facingRelative = blob->action->fighter.status.position.x < blob->sphere.origin.x ? +1.f : -1.f;
        const float facing = blob->facing == BlobFacing::Relative ? facingRelative : blob->facing == BlobFacing::Forward
                             ? float(blob->action->fighter.status.facing) : float(-blob->action->fighter.status.facing);

        if (blob->useSakuraiAngle == true)
        {
            const float angle = maths::radians(40.f / 360.f);
            const Vec3F offsetGround = Vec3F(facing, 0.f, 0.f) * blob->sphere.radius;
            const Vec3F offsetLaunch = Vec3F(std::cos(angle) * facing, std::sin(angle), 0.f) * blob->sphere.radius;

            const Vec4F pA = projViewMat * Vec4F(blob->sphere.origin, 1.f);
            const Vec4F pB = projViewMat * Vec4F(blob->sphere.origin + offsetGround, 1.f);
            const Vec4F pC = projViewMat * Vec4F(blob->sphere.origin + offsetLaunch, 1.f);

            lineData.insert(lineData.end(), { pA,pB, pB,pC, pC,pA });
        }
        else
        {
            const float angleA = maths::radians(blob->knockAngle / 360.f);
            const float angleB = maths::radians(blob->knockAngle / 360.f + 0.0625f);
            const float angleC = maths::radians(blob->knockAngle / 360.f - 0.0625f);
            const Vec3F offsetA = Vec3F(std::cos(angleA) * facing, std::sin(angleA), 0.f) * blob->sphere.radius;
            const Vec3F offsetB = Vec3F(std::cos(angleB) * facing, std::sin(angleB), 0.f) * blob->sphere.radius * 0.75f;
            const Vec3F offsetC = Vec3F(std::cos(angleC) * facing, std::sin(angleC), 0.f) * blob->sphere.radius * 0.75f;

            const Vec4F pA = projViewMat * Vec4F(blob->sphere.origin, 1.f);
            const Vec4F pB = projViewMat * Vec4F(blob->sphere.origin + offsetA, 1.f);
            const Vec4F pC = projViewMat * Vec4F(blob->sphere.origin + offsetB, 1.f);
            const Vec4F pD = projViewMat * Vec4F(blob->sphere.origin + offsetC, 1.f);

            lineData.insert(lineData.end(), { pA,pB, pB,pC, pB,pD });
        }
    }

    //--------------------------------------------------------//

    context.set_state(sq::BlendMode::Disable);

    context.set_line_width(4.f);

    context.bind_program(mLinesShader);
    context.bind_vertexarray(mLineVertexArray);

    mLinesShader.update(0, Vec3F(1.f, 1.f, 1.f));
    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * lineData.size()), lineData.data());

    context.draw_arrays(sq::DrawPrimitive::Lines, 0u, uint(lineData.size()));
}

//============================================================================//

void DebugRenderer::render_hurt_blobs(const std::vector<HurtBlob*>& blobs)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_framebuffer(renderer.FB_Resolve);

    context.set_state(sq::BlendMode::Alpha);
    context.set_state(sq::CullFace::Back);
    context.set_state(sq::DepthTest::Disable);

    context.bind_program(mBlobShader);

    mCapsuleMesh.apply_to_context(context);

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
        mCapsuleMesh.draw_submesh(context, 0u);

        mBlobShader.update(0, projViewMat * matrixB);
        mCapsuleMesh.draw_submesh(context, 1u);

        if (length > 0.00001f)
        {
            const Vec3F originM = (c.originA + c.originB) * 0.5f;
            const Mat4F matrixM = maths::transform(originM, rotation, Vec3F(c.radius, length, c.radius));

            mBlobShader.update(0, projViewMat * matrixM);
            mCapsuleMesh.draw_submesh(context, 2u);
        }
    }
}

//============================================================================//

void DebugRenderer::render_diamond(const Fighter& fighter)
{
    auto& context = renderer.context;

    const Mat4F projViewMat = renderer.get_camera().get_combo_matrix();

    //--------------------------------------------------------//

    context.bind_framebuffer(renderer.FB_Resolve);

    context.set_state(sq::BlendMode::Alpha);
    context.set_state(sq::CullFace::Back);
    context.set_state(sq::DepthTest::Disable);

    context.bind_program(mBlobShader);

    mDiamondMesh.apply_to_context(context);

    mBlobShader.update(2, 0.25f);

    //--------------------------------------------------------//

    const Vec3F translate = Vec3F(fighter.status.position + Vec2F(0.f, fighter.get_diamond().offsetCross), 0.f);
    const float scaleBottom = fighter.get_diamond().offsetCross;
    const float scaleTop = fighter.get_diamond().offsetTop - fighter.get_diamond().offsetCross;

    const Mat4F bottomMat = maths::transform(translate, Vec3F(fighter.get_diamond().halfWidth, scaleBottom, 0.1f));
    const Mat4F topMat = maths::transform(translate, Vec3F(fighter.get_diamond().halfWidth, scaleTop, 0.1f));

    mBlobShader.update(1, Vec3F(1.f, 1.f, 1.f));

    mBlobShader.update(0, projViewMat * bottomMat);
    mDiamondMesh.draw_submesh(context, 0u);

    mBlobShader.update(0, projViewMat * topMat);
    mDiamondMesh.draw_submesh(context, 1u);
}

//============================================================================//

void DebugRenderer::render_skeleton(const Fighter& fighter)
{
    auto& context = renderer.context;

    const Mat4F projViewModelMat = renderer.get_camera().get_combo_matrix() * fighter.get_model_matrix();

    //--------------------------------------------------------//

    context.bind_framebuffer(renderer.FB_Resolve);

    //--------------------------------------------------------//

    const std::vector<Mat4F> mats = fighter.get_armature().compute_skeleton_matrices(fighter.current.pose);

    std::vector<Vec4F> linesRed;
    std::vector<Vec4F> linesGreen;
    std::vector<Vec4F> linesBlue;
    std::vector<Vec4F> linesWhite;

    linesRed.reserve(mats.size() * 2u);
    linesGreen.reserve(mats.size() * 2u);
    linesBlue.reserve(mats.size() * 2u);
    linesWhite.reserve(mats.size() * 2u - 1u);

    //--------------------------------------------------------//

    for (uint i = 0u; i < mats.size(); ++i)
    {
        const Mat4F matrix = projViewModelMat * mats[i];

        linesRed.push_back(matrix[3]);
        linesRed.push_back((matrix * Vec4F(0.02f, 0.f, 0.f, 1.f)));
        linesGreen.push_back(matrix[3]);
        linesGreen.push_back((matrix * Vec4F(0.f, 0.02f, 0.f, 1.f)));
        linesBlue.push_back(matrix[3]);
        linesBlue.push_back((matrix * Vec4F(0.f, 0.f, 0.02f, 1.f)));

        if (int8_t parent = fighter.get_armature().get_bone_parent(i); parent != -1)
        {
            const Mat4F parentMatrix = projViewModelMat * mats[parent];
            linesWhite.push_back(parentMatrix[3]);
            linesWhite.push_back(matrix[3]);
        }
    }

    //--------------------------------------------------------//

    context.set_state(sq::BlendMode::Disable);
    context.set_state(sq::DepthTest::Disable);

    context.set_line_width(2.f);

    context.bind_program(mLinesShader);
    context.bind_vertexarray(mLineVertexArray);

    mLinesShader.update(0, Vec3F(1.f, 0.f, 0.f));
    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * linesRed.size()), linesRed.data());
    context.draw_arrays(sq::DrawPrimitive::Lines, 0u, uint(linesRed.size()));

    mLinesShader.update(0, Vec3F(0.f, 1.f, 0.f));
    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * linesGreen.size()), linesGreen.data());
    context.draw_arrays(sq::DrawPrimitive::Lines, 0u, uint(linesGreen.size()));

    mLinesShader.update(0, Vec3F(0.f, 0.f, 1.f));
    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * linesBlue.size()), linesBlue.data());
    context.draw_arrays(sq::DrawPrimitive::Lines, 0u, uint(linesBlue.size()));

    context.set_line_width(1.f);

    mLinesShader.update(0, Vec3F(1.f, 1.f, 1.f));
    mLineVertexBuffer.update(0u, uint(sizeof(Vec4F) * linesWhite.size()), linesWhite.data());
    context.draw_arrays(sq::DrawPrimitive::Lines, 0u, uint(linesWhite.size()));
}
