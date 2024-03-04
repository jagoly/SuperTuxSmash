#include "render/DebugRender.hpp"

#include "main/Options.hpp"

#include "game/Article.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"
#include "game/World.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/maths/Colours.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/objects/Armature.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

DebugRenderer::DebugRenderer(Renderer& renderer) : renderer(renderer)
{
    const auto& ctx = sq::VulkanContext::get();

    mBlobPipelineLayout = ctx.create_pipeline_layout (
        {}, {
            vk::PushConstantRange { vk::ShaderStageFlagBits::eVertex, 0u, 64u },
            vk::PushConstantRange { vk::ShaderStageFlagBits::eFragment, 64u, 16u },
        }
    );

    mPrimitivesPipelineLayout = ctx.create_pipeline_layout({}, {});

    mTrianglesVertexBuffer.initialise(sizeof(Triangle) * MAX_TRIANGLES, vk::BufferUsageFlagBits::eVertexBuffer);
    mThickLinesVertexBuffer.initialise(sizeof(Line) * MAX_THICK_LINES, vk::BufferUsageFlagBits::eVertexBuffer);
    mThinLinesVertexBuffer.initialise(sizeof(Line) * MAX_THIN_LINES, vk::BufferUsageFlagBits::eVertexBuffer);

    mSphereMesh.load_from_file("assets/debug/Sphere.sqm");
    mCapsuleMesh.load_from_file("assets/debug/Capsule.sqm");
}

//============================================================================//

DebugRenderer::~DebugRenderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mBlobPipelineLayout);
    ctx.device.destroy(mPrimitivesPipelineLayout);
}

//============================================================================//

void DebugRenderer::refresh_options_destroy()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mBlobPipeline);
    ctx.device.destroy(mTrianglesPipeline);
    ctx.device.destroy(mLinesPipeline);
}

//============================================================================//

void DebugRenderer::refresh_options_create()
{
    const auto& ctx = sq::VulkanContext::get();
    const Vec2U windowSize = renderer.window.get_size();

    // create blob pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/debug/Blob.vert.spv", {}, "shaders/debug/Blob.frag.spv"
        );

        const auto vertexConfig = sq::Mesh::VertexConfig({}, {}); // position only

        mBlobPipeline = sq::vk_create_graphics_pipeline (
            ctx, mBlobPipelineLayout, renderer.window.get_render_pass(), 0u, shaderModules.stages, vertexConfig.state,
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack,
                vk::FrontFace::eClockwise, false, 0.f, false, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
            },
            vk::PipelineDepthStencilStateCreateInfo {
                {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, float(windowSize.y), float(windowSize.x), -float(windowSize.y), 0.f, 1.f },
            vk::Rect2D { {0, 0}, {windowSize.x, windowSize.y} },
            vk::PipelineColorBlendAttachmentState {
                true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlags(0b1111)
            },
            nullptr
        );
    }

    // create primitive pipelines
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/debug/Primitives.vert.spv", {}, "shaders/debug/Primitives.frag.spv"
        );

        const auto vertexBindingDescriptions = std::array {
            vk::VertexInputBindingDescription { 0u, 32u, vk::VertexInputRate::eVertex }
        };

        const auto vertexAttributeDescriptions = std::array {
            vk::VertexInputAttributeDescription { 0u, 0u, vk::Format::eR32G32B32A32Sfloat, 0u },
            vk::VertexInputAttributeDescription { 1u, 0u, vk::Format::eR32G32B32A32Sfloat, 16u },
        };

        mTrianglesPipeline = sq::vk_create_graphics_pipeline (
            ctx, mPrimitivesPipelineLayout, renderer.window.get_render_pass(), 0u, shaderModules.stages,
            vk::PipelineVertexInputStateCreateInfo {
                {}, vertexBindingDescriptions, vertexAttributeDescriptions
            },
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise, false, 0.f, false, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
            },
            vk::PipelineDepthStencilStateCreateInfo {
                {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, float(windowSize.y), float(windowSize.x), -float(windowSize.y), 0.f, 1.f },
            vk::Rect2D { {0, 0}, {windowSize.x, windowSize.y} },
            vk::PipelineColorBlendAttachmentState {
                true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlags(0b1111)
            },
            nullptr
        );

        mLinesPipeline = sq::vk_create_graphics_pipeline (
            ctx, mPrimitivesPipelineLayout, renderer.window.get_render_pass(), 0u, shaderModules.stages,
            vk::PipelineVertexInputStateCreateInfo {
                {}, vertexBindingDescriptions, vertexAttributeDescriptions
            },
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eLineList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eLine, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise, false, 0.f, false, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
            },
            vk::PipelineDepthStencilStateCreateInfo {
                {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, float(windowSize.y), float(windowSize.x), -float(windowSize.y), 0.f, 1.f },
            vk::Rect2D { {0, 0}, {windowSize.x, windowSize.y} },
            vk::PipelineColorBlendAttachmentState {
                true, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlags(0b1111)
            },
            { vk::DynamicState::eLineWidth }
        );
    }
}

//============================================================================//

void DebugRenderer::integrate(float /*blend*/, const World& world)
{
    if (renderer.options.render_hurt_blobs == true)
        for (const auto& fighter : world.get_fighters())
            impl_integrate_hurt_blobs(fighter->get_hurt_blobs());

    if (renderer.options.render_hit_blobs == true)
    {
        for (const auto& fighter : world.get_fighters())
            impl_integrate_hit_blobs(fighter->get_hit_blobs());

        for (const auto& article : world.get_articles())
            impl_integrate_hit_blobs(article->get_hit_blobs());
    }

    if (renderer.options.render_diamonds == true)
        for (const auto& fighter : world.get_fighters())
            impl_integrate_diamond(*fighter);

    if (renderer.options.render_skeletons == true)
    {
        for (const auto& fighter : world.get_fighters())
            impl_integrate_skeleton(*fighter);

        for (const auto& article : world.get_articles())
            impl_integrate_skeleton(*article);
    }
}

//============================================================================//

void DebugRenderer::impl_integrate_hit_blobs(const std::vector<HitBlob>& blobs)
{
    const Mat4F& projViewMat = renderer.get_camera().get_block().projViewMat;

    Line* const thick = reinterpret_cast<Line*>(mThickLinesVertexBuffer.map_only());

    for (const HitBlob& blob : blobs)
    {
        const maths::Capsule& c = blob.capsule;

        const Vec3F difference = c.originB - c.originA;
        const float length = maths::length(difference);
        const Mat3F rotation = length > 0.00001f ? maths::basis_from_y(difference / length) : Mat3F();

        const Vec4F colour = Vec4F(maths::srgb_to_linear(blob.get_debug_colour()), 0.25f);

        DrawBlob& drawA = mDrawBlobs.emplace_back();
        drawA.matrix = projViewMat * maths::transform(c.originA, rotation, c.radius);
        drawA.colour = colour;
        drawA.mesh = &mCapsuleMesh;
        drawA.subMesh = 0;
        drawA.sortValue = int(blob.def.flavour);

        DrawBlob& drawB = mDrawBlobs.emplace_back();
        drawB.matrix = projViewMat * maths::transform(c.originB, rotation, c.radius);
        drawB.colour = colour;
        drawB.mesh = &mCapsuleMesh;
        drawB.subMesh = 1;
        drawB.sortValue = int(blob.def.flavour);

        if (length > 0.00001f)
        {
            const Vec3F origin = (c.originA + c.originB) * 0.5f;
            const Vec3F scale = Vec3F(c.radius, length, c.radius);

            DrawBlob& drawC = mDrawBlobs.emplace_back();
            drawC.matrix = projViewMat * maths::transform(origin, rotation, scale);
            drawC.colour = colour;
            drawC.mesh = &mCapsuleMesh;
            drawC.subMesh = 2;
            drawC.sortValue = int(blob.def.flavour);
        }

        // for relative facing, we just show an arrow in the most likely direction
        // todo: if game is paused, alternate direction every 0.5 seconds
        const float facing = blob.def.facingMode == BlobFacingMode::Forward ? +float(blob.entity->get_vars().facing) :
                             blob.def.facingMode == BlobFacingMode::Reverse ? -float(blob.entity->get_vars().facing) :
                             blob.entity->get_vars().position.x < blob.capsule.originA.x ? +1.f : -1.f;

        if (blob.def.type == BlobType::Grab)
        {
            const Vec4F pA = projViewMat * Vec4F(blob.capsule.originA + Vec3F(+0.4f, +0.5f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pB = projViewMat * Vec4F(blob.capsule.originA + Vec3F(-0.2f, +0.5f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pC = projViewMat * Vec4F(blob.capsule.originA + Vec3F(-0.4f, +0.2f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pD = projViewMat * Vec4F(blob.capsule.originA + Vec3F(-0.4f, -0.2f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pE = projViewMat * Vec4F(blob.capsule.originA + Vec3F(-0.2f, -0.5f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pF = projViewMat * Vec4F(blob.capsule.originA + Vec3F(+0.4f, -0.5f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pG = projViewMat * Vec4F(blob.capsule.originA + Vec3F(+0.4f,  0.0f, 0.f) * blob.capsule.radius, 1.f);
            const Vec4F pH = projViewMat * Vec4F(blob.capsule.originA + Vec3F(+0.1f,  0.0f, 0.f) * blob.capsule.radius, 1.f);

            thick[mThickLineCount++] = Line { pA, Vec4F(1,1,1,1), pB, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pC, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pC, Vec4F(1,1,1,1), pD, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pD, Vec4F(1,1,1,1), pE, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pE, Vec4F(1,1,1,1), pF, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pF, Vec4F(1,1,1,1), pG, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pG, Vec4F(1,1,1,1), pH, Vec4F(1,1,1,1) };
        }
        else if (blob.def.angleMode == BlobAngleMode::Sakurai)
        {
            const float angle = maths::radians(40.f / 360.f);
            const Vec3F offsetGround = Vec3F(facing, 0.f, 0.f) * blob.capsule.radius;
            const Vec3F offsetLaunch = Vec3F(std::cos(angle) * facing, std::sin(angle), 0.f) * blob.capsule.radius;

            const Vec4F pA = projViewMat * Vec4F(blob.capsule.originA, 1.f);
            const Vec4F pB = projViewMat * Vec4F(blob.capsule.originA + offsetGround, 1.f);
            const Vec4F pC = projViewMat * Vec4F(blob.capsule.originA + offsetLaunch, 1.f);

            thick[mThickLineCount++] = Line { pA, Vec4F(1,1,1,1), pB, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pC, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pC, Vec4F(1,1,1,1), pA, Vec4F(1,1,1,1) };
        }
        else // todo: AutoLink
        {
            const float angleA = maths::radians(blob.def.knockAngle / 360.f);
            const float angleB = maths::radians(blob.def.knockAngle / 360.f + 0.0625f);
            const float angleC = maths::radians(blob.def.knockAngle / 360.f - 0.0625f);
            const Vec3F offsetA = Vec3F(std::cos(angleA) * facing, std::sin(angleA), 0.f) * blob.capsule.radius;
            const Vec3F offsetB = Vec3F(std::cos(angleB) * facing, std::sin(angleB), 0.f) * blob.capsule.radius * 0.75f;
            const Vec3F offsetC = Vec3F(std::cos(angleC) * facing, std::sin(angleC), 0.f) * blob.capsule.radius * 0.75f;

            const Vec4F pA = projViewMat * Vec4F(blob.capsule.originA, 1.f);
            const Vec4F pB = projViewMat * Vec4F(blob.capsule.originA + offsetA, 1.f);
            const Vec4F pC = projViewMat * Vec4F(blob.capsule.originA + offsetB, 1.f);
            const Vec4F pD = projViewMat * Vec4F(blob.capsule.originA + offsetC, 1.f);

            thick[mThickLineCount++] = Line { pA, Vec4F(1,1,1,1), pB, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pC, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pD, Vec4F(1,1,1,1) };
        }
    }
}

//============================================================================//

void DebugRenderer::impl_integrate_hurt_blobs(const std::vector<HurtBlob>& blobs)
{
    const Mat4F& projViewMat = renderer.get_camera().get_block().projViewMat;

    for (const HurtBlob& blob : blobs)
    {
        const maths::Capsule& c = blob.capsule;

        const Vec3F difference = c.originB - c.originA;
        const float length = maths::length(difference);
        const Mat3F rotation = length > 0.00001f ? maths::basis_from_y(difference / length) : Mat3F();

        const Vec4F colour = Vec4F(maths::srgb_to_linear(blob.get_debug_colour()), 0.25f);

        DrawBlob& drawA = mDrawBlobs.emplace_back();
        drawA.matrix = projViewMat * maths::transform(c.originA, rotation, c.radius);
        drawA.colour = colour;
        drawA.mesh = &mCapsuleMesh;
        drawA.subMesh = 0;
        drawA.sortValue = int(blob.def.region) - 3;

        DrawBlob& drawB = mDrawBlobs.emplace_back();
        drawB.matrix = projViewMat * maths::transform(c.originB, rotation, c.radius);
        drawB.colour = colour;
        drawB.mesh = &mCapsuleMesh;
        drawB.subMesh = 1;
        drawB.sortValue = int(blob.def.region) - 3;

        if (length > 0.00001f)
        {
            const Vec3F origin = (c.originA + c.originB) * 0.5f;
            const Vec3F scale = Vec3F(c.radius, length, c.radius);

            DrawBlob& drawC = mDrawBlobs.emplace_back();
            drawC.matrix = projViewMat * maths::transform(origin, rotation, scale);
            drawC.colour = colour;
            drawC.mesh = &mCapsuleMesh;
            drawC.subMesh = 2;
            drawC.sortValue = int(blob.def.region) - 3;
        }
    }
}

//============================================================================//

void DebugRenderer::impl_integrate_diamond(const Fighter& fighter)
{
    const Mat4F& projViewMat = renderer.get_camera().get_block().projViewMat;

    Triangle* const triangles = reinterpret_cast<Triangle*>(mTrianglesVertexBuffer.map_only());

    const Diamond& dm = fighter.diamond;

    const Vec4F cross = projViewMat * Vec4F(dm.cross, 0.f, 1.f);
    const Vec4F pA = projViewMat * Vec4F(dm.cross.x, dm.max.y, 0.f, 1.f);
    const Vec4F pB = projViewMat * Vec4F(dm.max.x, dm.cross.y, 0.f, 1.f);
    const Vec4F pC = projViewMat * Vec4F(dm.cross.x, dm.min.y, 0.f, 1.f);
    const Vec4F pD = projViewMat * Vec4F(dm.min.x, dm.cross.y, 0.f, 1.f);

    const Vec4F colour = Vec4F(1.f, 1.f, 1.f, 0.2f);

    triangles[mTriangleCount++] = Triangle { cross, colour, pA, colour, pB, colour };
    triangles[mTriangleCount++] = Triangle { cross, colour, pB, colour, pC, colour };
    triangles[mTriangleCount++] = Triangle { cross, colour, pC, colour, pD, colour };
    triangles[mTriangleCount++] = Triangle { cross, colour, pD, colour, pA, colour };
}

//============================================================================//

void DebugRenderer::impl_integrate_skeleton(const Entity& entity)
{
    const Mat4F projViewModelMat = renderer.get_camera().get_block().projViewMat * entity.get_model_matrix(-1);

    const std::vector<Mat4F> mats = entity.get_armature().compute_bone_matrices(entity.get_anim_player().currentSample);

    Line* const thin = reinterpret_cast<Line*>(mThinLinesVertexBuffer.map_only());

    for (size_t i = 0u; i < mats.size(); ++i)
    {
        const Mat4F matrix = projViewModelMat * mats[i];
        thin[mThinLineCount++] = Line { matrix[3], Vec4F(1,0,0,1), matrix * Vec4F(0.03f,0,0,1), Vec4F(1,0,0,1) };
        thin[mThinLineCount++] = Line { matrix[3], Vec4F(0,1,0,1), matrix * Vec4F(0,0.03f,0,1), Vec4F(0,1,0,1) };
        thin[mThinLineCount++] = Line { matrix[3], Vec4F(0,0,1,1), matrix * Vec4F(0,0,0.03f,1), Vec4F(0,0,1,1) };

        if (int8_t parent = entity.get_armature().get_bone_infos()[i].parent; parent != -1)
        {
            const Mat4F parentMatrix = projViewModelMat * mats[parent];
            thin[mThinLineCount++] = Line { parentMatrix[3], Vec4F(1,1,1,1), matrix[3], Vec4F(1,1,1,1) };
        }
    }
}

//============================================================================//

void DebugRenderer::populate_command_buffer(vk::CommandBuffer cmdbuf)
{
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mTrianglesPipeline);
    cmdbuf.bindVertexBuffers(0u, mTrianglesVertexBuffer.front(), size_t(0u));
    cmdbuf.draw(mTriangleCount * 3u, 1u, 0u, 0u);

    //--------------------------------------------------------//

    // middle < lower < upper < sour < tangy < sweet
    const auto compare = [](const DrawBlob& a, const DrawBlob& b) { return a.sortValue < b.sortValue; };
    std::sort(mDrawBlobs.begin(), mDrawBlobs.end(), compare);

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mBlobPipeline);

    const sq::Mesh* boundMesh = nullptr;

    for (const DrawBlob& blob : mDrawBlobs)
    {
        if (boundMesh != blob.mesh)
        {
            blob.mesh->bind_buffers(cmdbuf);
            boundMesh = blob.mesh;
        }

        cmdbuf.pushConstants<Mat4F>(mBlobPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0u, blob.matrix);
        cmdbuf.pushConstants<Vec4F>(mBlobPipelineLayout, vk::ShaderStageFlagBits::eFragment, 64u, blob.colour);

        blob.mesh->draw(cmdbuf, blob.subMesh);
    }

    //--------------------------------------------------------//

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mLinesPipeline);

    cmdbuf.setLineWidth(2.f);
    cmdbuf.bindVertexBuffers(0u, mThickLinesVertexBuffer.front(), size_t(0u));
    cmdbuf.draw(mThickLineCount * 2u, 1u, 0u, 0u);

    cmdbuf.setLineWidth(1.f);
    cmdbuf.bindVertexBuffers(0u, mThinLinesVertexBuffer.front(), size_t(0u));
    cmdbuf.draw(mThinLineCount * 2u, 1u, 0u, 0u);

    //--------------------------------------------------------//

    mDrawBlobs.clear();

    mTrianglesVertexBuffer.swap_only();
    mThickLinesVertexBuffer.swap_only();
    mThinLinesVertexBuffer.swap_only();

    mTriangleCount = 0u;
    mThickLineCount = 0u;
    mThinLineCount = 0u;
}
