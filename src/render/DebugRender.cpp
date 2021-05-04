#include "render/DebugRender.hpp"

#include "main/Options.hpp"

#include "game/Action.hpp"
#include "game/Fighter.hpp"
#include "game/HitBlob.hpp"
#include "game/HurtBlob.hpp"

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
    //-- Load Mesh Objects -----------------------------------//

    mSphereMesh.load_from_file("assets/debug/Sphere.sqm", true);
    mCapsuleMesh.load_from_file("assets/debug/Capsule.sqm", true);
    mDiamondMesh.load_from_file("assets/debug/Diamond.sqm", true);

    //-- Set up the vao and vbo ------------------------------//

    mThickLinesVertexBuffer.initialise(sizeof(Vec4F) * 256u * 4u, vk::BufferUsageFlagBits::eVertexBuffer);
    mThinLinesVertexBuffer.initialise(sizeof(Vec4F) * 512u * 4u, vk::BufferUsageFlagBits::eVertexBuffer);

    const auto& ctx = sq::VulkanContext::get();

    // create blob pipeline layout
    {
        const auto pushConstantRanges = std::array {
            vk::PushConstantRange { vk::ShaderStageFlagBits::eVertex, 0u, 64u },
            vk::PushConstantRange { vk::ShaderStageFlagBits::eFragment, 64u, 16u },
        };

        mBlobPipelineLayout = ctx.device.createPipelineLayout (
            vk::PipelineLayoutCreateInfo { {}, {}, pushConstantRanges }
        );
    }

    // create lines pipeline layout
    {
        const auto pushConstantRanges = std::array {
            vk::PushConstantRange { vk::ShaderStageFlagBits::eFragment, 0u, 16u },
        };

        mLinesPipelineLayout = ctx.device.createPipelineLayout (
            vk::PipelineLayoutCreateInfo { {}, {}, pushConstantRanges }
        );
    }
}

//============================================================================//

DebugRenderer::~DebugRenderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mBlobPipelineLayout);
    ctx.device.destroy(mLinesPipelineLayout);
    ctx.device.destroy(mBlobPipeline);
    ctx.device.destroy(mLinesPipeline);
}

//============================================================================//

void DebugRenderer::refresh_options_destroy()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mBlobPipeline);
    ctx.device.destroy(mLinesPipeline);
}

void DebugRenderer::refresh_options_create()
{
    const auto& ctx = sq::VulkanContext::get();
    const Vec2U windowSize = renderer.window.get_size();

    // create blob pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/debug/Blob.vert.spv", {}, "shaders/debug/Blob.frag.spv"
        );

        const auto vertexConfig = sq::VulkMesh::VertexConfig({});

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
                true,
                vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
                vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
                vk::ColorComponentFlags(0b1111)
            },
            nullptr
        );
    }

    // create lines pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/debug/Lines.vert.spv", {}, "shaders/debug/Lines.frag.spv"
        );

        const auto vertexBindingDescriptions = std::array {
            vk::VertexInputBindingDescription { 0u, 32u, vk::VertexInputRate::eVertex }
        };

        const auto vertexAttributeDescriptions = std::array {
            vk::VertexInputAttributeDescription { 0u, 0u, vk::Format::eR32G32B32A32Sfloat, 0u },
            vk::VertexInputAttributeDescription { 1u, 0u, vk::Format::eR32G32B32A32Sfloat, 16u },
        };

        mLinesPipeline = sq::vk_create_graphics_pipeline (
            ctx, mLinesPipelineLayout, renderer.window.get_render_pass(), 0u, shaderModules.stages,
            vk::PipelineVertexInputStateCreateInfo {
                {}, vertexBindingDescriptions, vertexAttributeDescriptions
            },
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eLineList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eLine, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise, false, 0.f, false, 0.f, 2.f
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

void DebugRenderer::render_hit_blobs(const std::vector<HitBlob*>& blobs)
{
    const Mat4F projViewMat = renderer.get_camera().get_block().projViewMat;

    Line* thick = reinterpret_cast<Line*>(mThickLinesVertexBuffer.map_only());

    for (const HitBlob* blob : blobs)
    {
        DrawBlob& draw = mDrawBlobs.emplace_back();
        draw.matrix = projViewMat * maths::transform(blob->sphere.origin, Vec3F(blob->sphere.radius));
        draw.colour = Vec4F(maths::srgb_to_linear(blob->get_debug_colour()), 0.25f);
        draw.mesh = &mSphereMesh;
        draw.subMesh = -1;
        draw.sortValue = int(blob->flavour);

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

            thick[mThickLineCount++] = Line { pA, Vec4F(1,1,1,1), pB, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pC, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pC, Vec4F(1,1,1,1), pA, Vec4F(1,1,1,1) };
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

            thick[mThickLineCount++] = Line { pA, Vec4F(1,1,1,1), pB, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pC, Vec4F(1,1,1,1) };
            thick[mThickLineCount++] = Line { pB, Vec4F(1,1,1,1), pD, Vec4F(1,1,1,1) };
        }
    }
}

//============================================================================//

void DebugRenderer::render_hurt_blobs(const std::vector<HurtBlob*>& blobs)
{
    const Mat4F projViewMat = renderer.get_camera().get_block().projViewMat;

    for (const HurtBlob* blob : blobs)
    {
        const maths::Capsule& c = blob->capsule;

        const Vec3F difference = c.originB - c.originA;
        const float length = maths::length(difference);
        const Mat3F rotation = length > 0.00001f ? maths::basis_from_y(difference / length) : Mat3F();

        const Vec4F colour = Vec4F(maths::srgb_to_linear(blob->get_debug_colour()), 0.25f);

        DrawBlob& drawA = mDrawBlobs.emplace_back();
        drawA.matrix = projViewMat * maths::transform(c.originA, rotation, c.radius);
        drawA.colour = colour;
        drawA.mesh = &mCapsuleMesh;
        drawA.subMesh = 0;
        drawA.sortValue = int(blob->region) - 3;

        DrawBlob& drawB = mDrawBlobs.emplace_back();
        drawB.matrix = projViewMat * maths::transform(c.originB, rotation, c.radius);
        drawB.colour = colour;
        drawB.mesh = &mCapsuleMesh;
        drawB.subMesh = 1;
        drawB.sortValue = int(blob->region) - 3;

        if (length > 0.00001f)
        {
            const Vec3F origin = (c.originA + c.originB) * 0.5f;
            const Vec3F scale = Vec3F(c.radius, length, c.radius);

            DrawBlob& drawC = mDrawBlobs.emplace_back();
            drawC.matrix = projViewMat * maths::transform(origin, rotation, scale);
            drawC.colour = colour;
            drawC.mesh = &mCapsuleMesh;
            drawC.subMesh = 2;
            drawC.sortValue = int(blob->region) - 3;
        }
    }
}

//============================================================================//

void DebugRenderer::render_diamond(const Fighter& fighter)
{
    const Mat4F projViewMat = renderer.get_camera().get_block().projViewMat;

    const Vec3F translate = Vec3F(fighter.status.position + Vec2F(0.f, fighter.get_diamond().offsetCross), 0.f);
    const float scaleBtm = fighter.get_diamond().offsetCross;
    const float scaleTop = fighter.get_diamond().offsetTop - fighter.get_diamond().offsetCross;

    DrawBlob& drawBtm = mDrawBlobs.emplace_back();
    drawBtm.matrix = projViewMat * maths::transform(translate, Vec3F(fighter.get_diamond().halfWidth, scaleBtm, 0.1f));
    drawBtm.colour = maths::srgb_to_linear(Vec4F(1.f, 1.f, 1.f, 0.25f));
    drawBtm.mesh = &mDiamondMesh;
    drawBtm.subMesh = 0;
    drawBtm.sortValue = -4;

    DrawBlob& drawTop = mDrawBlobs.emplace_back();
    drawTop.matrix = projViewMat * maths::transform(translate, Vec3F(fighter.get_diamond().halfWidth, scaleTop, 0.1f));
    drawTop.colour = maths::srgb_to_linear(Vec4F(1.f, 1.f, 1.f, 0.25f));
    drawTop.mesh = &mDiamondMesh;
    drawTop.subMesh = 1;
    drawTop.sortValue = -4;
}

//============================================================================//

void DebugRenderer::render_skeleton(const Fighter& fighter)
{
    const Mat4F projViewModelMat = renderer.get_camera().get_block().projViewMat * fighter.get_model_matrix();

    const std::vector<Mat4F> mats = fighter.get_armature().compute_skeleton_matrices(fighter.current.pose);

    Line* thick = reinterpret_cast<Line*>(mThickLinesVertexBuffer.map_only());
    Line* thin = reinterpret_cast<Line*>(mThinLinesVertexBuffer.map_only());

    for (uint i = 0u; i < mats.size(); ++i)
    {
        const Mat4F matrix = projViewModelMat * mats[i];
        thin[mThinLineCount++] = Line { matrix[3], Vec4F(1,0,0,1), matrix * Vec4F(0.03f,0,0,1), Vec4F(1,0,0,1) };
        thin[mThinLineCount++] = Line { matrix[3], Vec4F(0,1,0,1), matrix * Vec4F(0,0.03f,0,1), Vec4F(0,1,0,1) };
        thin[mThinLineCount++] = Line { matrix[3], Vec4F(0,0,1,1), matrix * Vec4F(0,0,0.03f,1), Vec4F(0,0,1,1) };

        if (int8_t parent = fighter.get_armature().get_bone_parent(i); parent != -1)
        {
            const Mat4F parentMatrix = projViewModelMat * mats[parent];
            thick[mThickLineCount++] = Line { parentMatrix[3], Vec4F(1,1,1,1), matrix[3], Vec4F(1,1,1,1) };
        }
    }
}

//============================================================================//

void DebugRenderer::populate_command_buffer(vk::CommandBuffer cmdbuf)
{
    // diamond < middle < lower < upper < sour < tangy < sweet
    std::sort(mDrawBlobs.begin(), mDrawBlobs.end());

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mBlobPipeline);

    const sq::VulkMesh* boundMesh = nullptr;

    for (const DrawBlob& blob : mDrawBlobs)
    {
        if (boundMesh != blob.mesh)
            blob.mesh->bind_buffers(cmdbuf);
                boundMesh = blob.mesh;

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

    mThickLineCount = 0u;
    mThinLineCount = 0u;

    mThickLinesVertexBuffer.swap_only();
    mThinLinesVertexBuffer.swap_only();
}
