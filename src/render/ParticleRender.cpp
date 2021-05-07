#include "render/ParticleRender.hpp"

#include "main/Options.hpp"
#include "render/Renderer.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/vk/VulkanContext.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

ParticleRenderer::ParticleRenderer(Renderer& renderer) : renderer(renderer)
{
    mVertexBuffer.initialise(sizeof(ParticleVertex) * 8192u, vk::BufferUsageFlagBits::eVertexBuffer);

    const auto& ctx = sq::VulkanContext::get();

    mDescriptorSetLayout = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
            vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eInputAttachment, 1u, vk::ShaderStageFlagBits::eFragment }
        }
    );

    mPipelineLayout = sq::vk_create_pipeline_layout (
        ctx, {}, { renderer.setLayouts.camera, mDescriptorSetLayout }, {}
    );

    mTexture.load_from_file_array("assets/particles/Basic128");

    mDescriptorSet = sq::vk_allocate_descriptor_set(ctx, mDescriptorSetLayout);
    sq::vk_update_descriptor_set(ctx, mDescriptorSet, 0u, 0u, vk::DescriptorType::eCombinedImageSampler, mTexture.get_descriptor_info());
}

//============================================================================//

ParticleRenderer::~ParticleRenderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mDescriptorSetLayout);
    ctx.device.free(ctx.descriptorPool, mDescriptorSet);
    ctx.device.destroy(mPipelineLayout);
    ctx.device.destroy(mPipeline);
}

//============================================================================//

void ParticleRenderer::refresh_options_destroy()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mPipeline);
}

void ParticleRenderer::refresh_options_create()
{
    const auto& ctx = sq::VulkanContext::get();
    const Vec2U windowSize = renderer.window.get_size();
    const auto msaaMode = vk::SampleCountFlagBits(renderer.options.msaa_quality);

    // create pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/particles/Generic.vert.spv", "shaders/particles/Generic.geom.spv",
            "shaders/particles/Generic{}x.frag.spv"_format(int(msaaMode))
        );

        const auto vertexBindingDescriptions = std::array {
            vk::VertexInputBindingDescription { 0u, sizeof(ParticleVertex), vk::VertexInputRate::eVertex }
        };

        const auto vertexAttributeDescriptions = std::array {
            vk::VertexInputAttributeDescription { 0u, 0u, vk::Format::eR32G32B32Sfloat, 0u },
            vk::VertexInputAttributeDescription { 1u, 0u, vk::Format::eR32Sfloat, 12u },
            vk::VertexInputAttributeDescription { 2u, 0u, vk::Format::eR16G16B16Unorm, 16u },
            vk::VertexInputAttributeDescription { 3u, 0u, vk::Format::eR16Unorm, 22u },
            vk::VertexInputAttributeDescription { 4u, 0u, vk::Format::eR32Sfloat, 24u },
            vk::VertexInputAttributeDescription { 5u, 0u, vk::Format::eR32Sfloat, 28u },
        };

        mPipeline = sq::vk_create_graphics_pipeline (
            ctx, mPipelineLayout, renderer.targets.msRenderPass, 1u, shaderModules.stages,
            vk::PipelineVertexInputStateCreateInfo {
                {}, vertexBindingDescriptions, vertexAttributeDescriptions
            },
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::ePointList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise, false, 0.f, false, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, msaaMode, false, 0.f, nullptr, false, false
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

        sq::vk_update_descriptor_set (
            ctx, mDescriptorSet, 1u, 0u, vk::DescriptorType::eInputAttachment,
            vk::DescriptorImageInfo {
                {}, int(msaaMode) > 1 ? renderer.images.msDepthView : renderer.images.resolveDepthView,
                vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            }
        );
    }
}

//============================================================================//

void ParticleRenderer::swap_sets()
{
    mParticleSetInfoKeep.clear();
    std::swap(mParticleSetInfo, mParticleSetInfoKeep);

    mVertices.clear();
}

void ParticleRenderer::integrate_set(float blend, const ParticleSystem& system)
{
    auto& info = mParticleSetInfo.emplace_back();

    //info.texture = renderer.resources.texarrays.acquire(system.texturePath);
    info.startIndex = uint16_t(mVertices.size());
    info.vertexCount = uint16_t(system.get_vertex_count());

    mVertices.reserve(mVertices.size() + system.get_vertex_count());
    system.compute_vertices(blend, mVertices);
}

//============================================================================//

void ParticleRenderer::populate_command_buffer(vk::CommandBuffer cmdbuf)
{
    std::memcpy(mVertexBuffer.swap_map(), mVertices.data(), mVertices.size() * sizeof(ParticleVertex));

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 1u, mDescriptorSet, {});
    cmdbuf.bindVertexBuffers(0u, mVertexBuffer.front(), size_t(0u));
    cmdbuf.draw(mVertices.size(), 1u, 0u, 0u);

//    mVertexBuffer.update(0u, sizeof(ParticleVertex) * uint(mVertices.size()), mVertices.data());

//    const auto compare = [](ParticleSetInfo& a, ParticleSetInfo& b) { return a.averageDepth > b.averageDepth; };
//    std::sort(mParticleSetInfo.begin(), mParticleSetInfo.end(), compare);

//    //--------------------------------------------------------//

//    auto& context = renderer.context;

//    context.bind_framebuffer(renderer.FB_Resolve);

//    context.bind_vertexarray(mVertexArray);
//    context.bind_program(renderer.PROG_Particles);

//    //gl::Enable(gl::PROGRAM_POINT_SIZE);

//    context.set_state(sq::BlendMode::Alpha);
//    context.set_state(sq::CullFace::Disable);
//    context.set_state(sq::DepthTest::Disable);
//    //context.set_state(Context::Depth_Compare::LessEqual);

//    context.bind_texture(mTexture, 0u);

//    context.bind_texture(renderer.TEX_Depth, 1u);

//    for (const ParticleSetInfo& info : mParticleSetInfo)
//    {
//        if (info.vertexCount == 0u) continue;

//        //context.bind_texture(info.texture.get(), 0u);
//        context.draw_arrays(sq::DrawPrimitive::Points, info.startIndex, info.vertexCount);
//    }
}
