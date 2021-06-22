#include "render/ParticleRender.hpp"

#include "main/Options.hpp"
#include "game/ParticleSystem.hpp"
#include "render/Renderer.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

ParticleRenderer::ParticleRenderer(Renderer& renderer) : renderer(renderer)
{
    const auto& ctx = sq::VulkanContext::get();

    mDescriptorSetLayout = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });

    mPipelineLayout = ctx.create_pipeline_layout (
        { renderer.setLayouts.camera, mDescriptorSetLayout }, {}
    );

    mVertexBuffer.initialise(sizeof(ParticleVertex) * MAX_PARTICLES, vk::BufferUsageFlagBits::eVertexBuffer);

    mTexture.load_from_file_array("assets/particles/Basic128");

    mDescriptorSet = sq::vk_allocate_descriptor_set(ctx, mDescriptorSetLayout);
    sq::vk_update_descriptor_set (
        ctx, mDescriptorSet, sq::DescriptorImageSampler(0u, 0u, mTexture.get_descriptor_info())
    );
}

//============================================================================//

ParticleRenderer::~ParticleRenderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mDescriptorSetLayout);
    ctx.device.free(ctx.descriptorPool, mDescriptorSet);
    ctx.device.destroy(mPipelineLayout);
}

//============================================================================//

void ParticleRenderer::refresh_options_destroy()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(mPipeline);
}

//============================================================================//

void ParticleRenderer::refresh_options_create()
{
    const auto& ctx = sq::VulkanContext::get();
    const auto windowSize = renderer.window.get_size();

    // create pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/particles/Generic.vert.spv", "shaders/particles/Generic.geom.spv", "shaders/particles/Generic.frag.spv"
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
            ctx, mPipelineLayout, renderer.passes.hdr.pass, 0u, shaderModules.stages,
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

        sq::vk_update_descriptor_set (
            ctx, mDescriptorSet,
            sq::DescriptorImageSampler (
                1u, 0u, renderer.samplers.nearestClamp, renderer.images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            )
        );
    }
}

//============================================================================//

void ParticleRenderer::integrate(float blend, const ParticleSystem& system)
{
    SQASSERT(system.get_particles().size() <= MAX_PARTICLES, "too many particles for vertex buffer");

    const auto UNorm16 = [](float value) { return uint16_t(value * 65535.0f); };
    ParticleVertex* const vertices = reinterpret_cast<ParticleVertex*>(mVertexBuffer.swap_map());
    mVertexCount = 0u;

    for (const ParticleData& p : system.get_particles())
    {
        ParticleVertex& vertex = vertices[mVertexCount++];

        // particles get simulated on the tick they are spawned, so progress will start at 1
        const float factor = (float(p.progress - 1u) + blend) / float(p.lifetime);

        vertex.position = maths::mix(p.previousPos, p.currentPos, blend);
        vertex.radius = p.baseRadius * maths::mix(1.f, p.endScale, factor);
        vertex.colour[0] = UNorm16(p.colour.r);
        vertex.colour[1] = UNorm16(p.colour.g);
        vertex.colour[2] = UNorm16(p.colour.b);
        //vertex.opacity = UNorm16(std::pow(p.baseOpacity * maths::mix(1.f, p.endOpacity, factor), 0.5f));
        vertex.opacity = UNorm16(p.baseOpacity * maths::mix(1.f, p.endOpacity, factor));
        vertex.index = float(p.sprite);
        vertex.padding = 0.f;
    }
}

//============================================================================//

void ParticleRenderer::populate_command_buffer(vk::CommandBuffer cmdbuf)
{
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mPipelineLayout, 1u, mDescriptorSet, {});
    cmdbuf.bindVertexBuffers(0u, mVertexBuffer.front(), size_t(0u));

    cmdbuf.draw(mVertexCount, 1u, 0u, 0u);
}
