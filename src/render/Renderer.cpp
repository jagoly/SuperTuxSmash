#include "render/Renderer.hpp"

#include "main/Options.hpp"

#include "render/AnimPlayer.hpp"
#include "render/Camera.hpp"
#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Mesh.hpp>
#include <sqee/objects/Pipeline.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

Renderer::Renderer(const sq::Window& window, const Options& options, ResourceCaches& caches)
    : window(window), options(options), caches(caches)
{
    impl_initialise_layouts();

    mPassConfigGbuffer = &caches.passConfigMap["Opaque"];
    mPassConfigShadowFront = &caches.passConfigMap["ShadowFront"];
    mPassConfigShadowBack = &caches.passConfigMap["ShadowBack"];
    mPassConfigTransparent = &caches.passConfigMap["Transparent"];

    const auto& ctx = sq::VulkanContext::get();

    // allocate fixed layout descriptor sets
    {
        sets.gbuffer = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.gbuffer);
        sets.shadow = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.shadow);
        sets.shadowMiddle = sq::vk_allocate_descriptor_set(ctx, setLayouts.shadowMiddle);
        sets.ssao = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.ssao);
        sets.ssaoBlur = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.ssaoBlur);
        sets.skybox = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.skybox);
        sets.transparent = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.transparent);
        sets.composite = sq::vk_allocate_descriptor_set(ctx, setLayouts.composite);
    }

    // initialise camera and environment uniform buffers
    {
        ubos.camera.initialise(sizeof(CameraBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        ubos.environment.initialise(sizeof(EnvironmentBlock), vk::BufferUsageFlagBits::eUniformBuffer);

        ubos.matrices.initialise(65536u, vk::BufferUsageFlagBits::eStorageBuffer);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.gbuffer,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info()),
            sq::DescriptorStorageBuffer(1u, 0u, ubos.matrices.get_descriptor_info())
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.shadow,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.environment.get_descriptor_info()),
            sq::DescriptorStorageBuffer(1u, 0u, ubos.matrices.get_descriptor_info())
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.skybox,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info())
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.transparent,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info()),
            sq::DescriptorStorageBuffer(1u, 0u, ubos.matrices.get_descriptor_info())
        );
    }

    // create immutable samplers
    {
        samplers.nearestClamp = ctx.create_sampler (
            vk::Filter::eNearest, vk::Filter::eNearest, {},
            vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
            0.f, 0u, 0u, false, false, {}
        );

        samplers.linearClamp = ctx.create_sampler (
            vk::Filter::eLinear, vk::Filter::eLinear, {},
            vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
            0.f, 0u, 0u, false, false, {}
        );
    }

    mDebugRenderer = std::make_unique<DebugRenderer>(*this);
    mParticleRenderer = std::make_unique<ParticleRenderer>(*this);

    mLutTexture.load_from_file_2D("assets/BrdfLut512");

    mTimestampQueryPool.front = ctx.device.createQueryPool({ {}, vk::QueryType::eTimestamp, NUM_TIME_STAMPS + 1u, {} });
    mTimestampQueryPool.back = ctx.device.createQueryPool({ {}, vk::QueryType::eTimestamp, NUM_TIME_STAMPS + 1u, {} });

    refresh_options_create();
}

//============================================================================//

Renderer::~Renderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.gbuffer);
    ctx.device.destroy(setLayouts.shadow);
    ctx.device.destroy(setLayouts.shadowMiddle);
    ctx.device.destroy(setLayouts.depthMipGen);
    ctx.device.destroy(setLayouts.ssao);
    ctx.device.destroy(setLayouts.ssaoBlur);
    ctx.device.destroy(setLayouts.skybox);
    ctx.device.destroy(setLayouts.transparent);
    ctx.device.destroy(setLayouts.composite);

    ctx.device.free(ctx.descriptorPool, {sets.gbuffer.front, sets.gbuffer.back});
    ctx.device.free(ctx.descriptorPool, {sets.shadow.front, sets.shadow.back});
    ctx.device.free(ctx.descriptorPool, sets.shadowMiddle);
    ctx.device.free(ctx.descriptorPool, {sets.ssao.front, sets.ssao.back});
    ctx.device.free(ctx.descriptorPool, {sets.ssaoBlur.front, sets.ssaoBlur.back});
    ctx.device.free(ctx.descriptorPool, {sets.skybox.front, sets.skybox.back});
    ctx.device.free(ctx.descriptorPool, {sets.transparent.front, sets.transparent.back});
    ctx.device.free(ctx.descriptorPool, sets.composite);

    ctx.device.destroy(pipelineLayouts.gbuffer);
    ctx.device.destroy(pipelineLayouts.shadow);
    ctx.device.destroy(pipelineLayouts.shadowMiddle);
    ctx.device.destroy(pipelineLayouts.depthMipGen);
    ctx.device.destroy(pipelineLayouts.ssao);
    ctx.device.destroy(pipelineLayouts.ssaoBlur);
    ctx.device.destroy(pipelineLayouts.skybox);
    ctx.device.destroy(pipelineLayouts.transparent);
    ctx.device.destroy(pipelineLayouts.composite);

    ctx.device.destroy(mTimestampQueryPool.front);
    ctx.device.destroy(mTimestampQueryPool.back);

    ctx.device.destroy(samplers.nearestClamp);
    ctx.device.destroy(samplers.linearClamp);

    refresh_options_destroy();
}

//============================================================================//

void Renderer::impl_initialise_layouts()
{
    const auto& ctx = sq::VulkanContext::get();

    setLayouts.gbuffer = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eStorageBuffer, 1u, vk::ShaderStageFlagBits::eVertex }
    });
    setLayouts.shadow = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eStorageBuffer, 1u, vk::ShaderStageFlagBits::eVertex }
    });
    setLayouts.shadowMiddle = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eInputAttachment, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eInputAttachment, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.depthMipGen = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.ssao = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 3u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.ssaoBlur = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.skybox = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.transparent = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eStorageBuffer, 1u, vk::ShaderStageFlagBits::eVertex },
        vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.composite = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });

    const auto drawItemPushConstants = vk::PushConstantRange(vk::ShaderStageFlagBits::eAllGraphics, 0u, 128u);
    const auto compositePushConstants = vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 0u, sizeof(tonemap));

    pipelineLayouts.gbuffer      = ctx.create_pipeline_layout({setLayouts.gbuffer, caches.bindlessTextureSetLayout}, drawItemPushConstants);
    pipelineLayouts.shadow       = ctx.create_pipeline_layout({setLayouts.shadow, caches.bindlessTextureSetLayout}, drawItemPushConstants);
    pipelineLayouts.shadowMiddle = ctx.create_pipeline_layout(setLayouts.shadowMiddle, {});
    pipelineLayouts.depthMipGen  = ctx.create_pipeline_layout(setLayouts.depthMipGen, {});
    pipelineLayouts.ssao         = ctx.create_pipeline_layout(setLayouts.ssao, {});
    pipelineLayouts.ssaoBlur     = ctx.create_pipeline_layout(setLayouts.ssaoBlur, {});
    pipelineLayouts.skybox       = ctx.create_pipeline_layout(setLayouts.skybox, {});
    pipelineLayouts.transparent  = ctx.create_pipeline_layout({setLayouts.transparent, caches.bindlessTextureSetLayout}, drawItemPushConstants);
    pipelineLayouts.composite    = ctx.create_pipeline_layout(setLayouts.composite, compositePushConstants);
}

//============================================================================//

void Renderer::impl_create_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    const Vec2U renderSize = window.get_size() / 2u * 2u;

    // create images and samplers
    {
        images.depthStencil.initialise_2D (
            ctx, vk::Format::eD24UnormS8Uint, renderSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil
        );

        images.depthView = ctx.create_image_view (
            images.depthStencil.image, vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint,
            {}, vk::ImageAspectFlagBits::eDepth, 0u, 1u, 0u, 1u
        );

        images.albedoRoughness.initialise_2D (
            ctx, vk::Format::eB8G8R8A8Srgb, renderSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eColor
        );

        images.normalMetallic.initialise_2D (
            ctx, vk::Format::eR16G16B16A16Snorm, renderSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eColor
        );

        images.colour.initialise_2D (
            ctx, vk::Format::eR16G16B16A16Sfloat, renderSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eColor
        );

        ctx.set_debug_object_name(images.depthStencil.image, "Renderer.images.depthStencil");
        ctx.set_debug_object_name(images.depthStencil.view, "Renderer.images.depthStencilView");
        ctx.set_debug_object_name(images.depthView, "Renderer.images.depthView");
        ctx.set_debug_object_name(images.albedoRoughness.image, "Renderer.images.albedoRoughness");
        ctx.set_debug_object_name(images.albedoRoughness.view, "Renderer.images.albedoRoughnessView");
        ctx.set_debug_object_name(images.normalMetallic.image, "Renderer.images.normalMetallic");
        ctx.set_debug_object_name(images.normalMetallic.view, "Renderer.images.normalMetallicView");
        ctx.set_debug_object_name(images.colour.image, "Renderer.images.colour");
        ctx.set_debug_object_name(images.colour.view, "Renderer.images.colourView");
    }

    // create gbuffer render pass and framebuffer
    {
        const auto attachments = std::array {
            vk::AttachmentDescription { // albedo, roughness
                {}, vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
            },
            vk::AttachmentDescription { // normal, metalic
                {}, vk::Format::eR16G16B16A16Snorm, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
            },
            vk::AttachmentDescription { // depth, stencil
                {}, vk::Format::eD24UnormS8Uint, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            }
        };

        const auto albedoRoughnessRefernce = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };
        const auto normalMetallicReference = vk::AttachmentReference { 1u, vk::ImageLayout::eColorAttachmentOptimal };
        const auto depthStencilReference   = vk::AttachmentReference { 2u, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        const auto colourReferences = std::array { albedoRoughnessRefernce, normalMetallicReference };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, colourReferences, nullptr, &depthStencilReference, nullptr
        };

        const auto dependency = vk::SubpassDependency {
            0u, VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eDepthStencilAttachmentRead,
            vk::DependencyFlagBits::eByRegion
        };

        passes.gbuffer.initialise (
            ctx, attachments, subpass, dependency, renderSize, 1u,
            { images.albedoRoughness.view, images.normalMetallic.view, images.depthStencil.view }
        );

        ctx.set_debug_object_name(passes.gbuffer.pass, "Renderer.RenderPass.Gbuffer");
        ctx.set_debug_object_name(passes.gbuffer.framebuf, "Renderer.Framebuffer.Gbuffer");
    }

    // create hdr renderpass and framebuffer
    {
        const auto attachments = std::array {
            vk::AttachmentDescription {
                {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
            },
            vk::AttachmentDescription {
                {}, vk::Format::eD24UnormS8Uint, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            }
        };

        const auto colourReference       = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };
        const auto depthStencilReference = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilReadOnlyOptimal };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, colourReference, nullptr, &depthStencilReference, nullptr
        };

        const auto dependencies = std::array {
            vk::SubpassDependency {
                0u, 0u,
                vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eLateFragmentTests,
                vk::AccessFlagBits::eDepthStencilAttachmentRead, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::DependencyFlagBits::eByRegion
            },
            vk::SubpassDependency {
                0u, VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            }
        };

        passes.hdr.initialise (
            ctx, attachments, subpass, dependencies, renderSize, 1u,
            { images.colour.view, images.depthStencil.view }
        );

        ctx.set_debug_object_name(passes.hdr.pass, "Renderer.RenderPass.HDR");
        ctx.set_debug_object_name(passes.hdr.framebuf, "Renderer.Framebuffer.HDR");
    }

    sq::vk_update_descriptor_set_swapper (
        ctx, sets.transparent,
        sq::DescriptorImageSampler(2u, 0u, samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
    );

    *mPassConfigGbuffer = sq::PassConfig {
        passes.gbuffer.pass, 0u, vk::SampleCountFlagBits::e1,
        vk::StencilOpState { {}, vk::StencilOp::eReplace, {}, vk::CompareOp::eAlways, 0, 0b1, 0b1 },
        window.get_size(), pipelineLayouts.gbuffer,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };

    *mPassConfigTransparent = sq::PassConfig {
        passes.hdr.pass, 0u, vk::SampleCountFlagBits::e1, vk::StencilOpState(),
        window.get_size(), pipelineLayouts.transparent,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };
}

//============================================================================//

void Renderer::impl_destroy_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    images.depthStencil.destroy(ctx);
    images.albedoRoughness.destroy(ctx);
    images.normalMetallic.destroy(ctx);
    images.colour.destroy(ctx);

    ctx.device.destroy(images.depthView);

    passes.gbuffer.destroy(ctx);
    passes.hdr.destroy(ctx);
}

//============================================================================//

void Renderer::impl_create_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    const Vec2U renderSize = window.get_size() / 2u * 2u;
    const uint shadowSize = SHADOW_MAP_BASE_SIZE * std::min(uint(options.shadow_quality), 2u);

    // skybox pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/Skybox.vert.spv", {}, "shaders/Skybox.frag.spv"
        );

        pipelines.skybox = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.skybox, passes.hdr.pass, 0u, shaderModules.stages, {},
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
                {}, false, false, {}, false, true, vk::StencilOpState({}, {}, {}, vk::CompareOp::eEqual, 0b1, 0, 0b0), {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, 0.f, float(renderSize.x), float(renderSize.y) },
            vk::Rect2D { {0, 0}, {renderSize.x, renderSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        ctx.set_debug_object_name(pipelines.skybox, "Renderer.pipelines.skybox");
    }

    // default light pipeline
    {
        std::vector<vk::DescriptorSetLayoutBinding> setLayoutBindings;
        setLayoutBindings.reserve(11u);

        setLayoutBindings.emplace_back(0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(1u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(3u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(4u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(5u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(6u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        setLayoutBindings.emplace_back(7u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);

        if (options.shadow_quality != 0u)
        {
            setLayoutBindings.emplace_back(8u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        }

        if (options.ssao_quality != 0u)
        {
            setLayoutBindings.emplace_back(9u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
            setLayoutBindings.emplace_back(10u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment);
        }

        setLayouts.lighting = ctx.create_descriptor_set_layout(setLayoutBindings);

        pipelineLayouts.lighting = ctx.create_pipeline_layout(setLayouts.lighting, {});

        const auto specialise = sq::SpecialisationInfo (
             float(RADIANCE_LEVELS - 1u), 1.f / float(shadowSize), int(options.shadow_quality)
        );

        String fragShaderPath = "shaders/lights/Default/Frag";
        if (options.shadow_quality > 0u) fragShaderPath += "_Shadow";
        if (options.ssao_quality > 0u) fragShaderPath += "_SSAO";
        fragShaderPath += ".frag.spv";

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenTC.vert.spv", {}, fragShaderPath, &specialise.info
        );

        pipelines.lighting = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.lighting, passes.hdr.pass, 0u, shaderModules.stages, {},
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
                {}, false, false, {}, false, true, vk::StencilOpState({}, {}, {}, vk::CompareOp::eEqual, 0b1, 0, 0b1), {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, 0.f, float(renderSize.x), float(renderSize.y) },
            vk::Rect2D { {0, 0}, {renderSize.x, renderSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        sets.lighting = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.lighting);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lighting,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info()),
            sq::DescriptorUniformBuffer(1u, 0u, ubos.environment.get_descriptor_info()),
            sq::DescriptorImageSampler(2u, 0u, mLutTexture.get_descriptor_info()),
            sq::DescriptorImageSampler(5u, 0u, samplers.nearestClamp, images.albedoRoughness.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(6u, 0u, samplers.nearestClamp, images.normalMetallic.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(7u, 0u, samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
        );

        if (options.shadow_quality != 0u)
            sq::vk_update_descriptor_set_swapper (
                ctx, sets.lighting,
                sq::DescriptorImageSampler(8u, 0u, samplers.depthCompare, images.shadowMiddle.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
            );

        if (options.ssao_quality != 0u)
            sq::vk_update_descriptor_set_swapper (
                ctx, sets.lighting,
                sq::DescriptorImageSampler(9u, 0u, samplers.linearClamp, images.ssaoBlur.view, vk::ImageLayout::eShaderReadOnlyOptimal),
                sq::DescriptorImageSampler(10u, 0u, samplers.nearestClamp, images.depthMips.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
            );

        ctx.set_debug_object_name(pipelines.lighting, "Renderer.Pipeline.Lighting");
    }

    // composite pipeline
    {
        const auto specialise = sq::SpecialisationInfo (
            options.debug_texture == "NoToneMap" || options.debug_texture == "Albedo" ? 1 :
            options.debug_texture == "Roughness" || options.debug_texture == "Metallic" ? 2 :
            options.debug_texture == "Depth" || options.debug_texture == "Shadow" ? 3 :
            options.debug_texture == "SSAO" && options.ssao_quality != 0u ? 3 :
            options.debug_texture == "Normal" ? 4 : 0
        );

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenTC.vert.spv", {}, "shaders/Composite.frag.spv", &specialise.info
        );

        pipelines.composite = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.composite, window.get_render_pass(), 0u, shaderModules.stages, {},
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise, false, 0.f, 0.f, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
            },
            vk::PipelineDepthStencilStateCreateInfo {
                {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, 0.f, float(window.get_size().x), float(window.get_size().y) },
            vk::Rect2D { {0, 0}, {window.get_size().x, window.get_size().y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        sq::DescriptorImageSampler descriptor = {
            0u, 0u, samplers.nearestClamp, images.colour.view, vk::ImageLayout::eShaderReadOnlyOptimal
        };

        if (options.debug_texture == "Albedo" || options.debug_texture == "Roughness")
            descriptor = { 0u, 0u, samplers.nearestClamp, images.albedoRoughness.view, vk::ImageLayout::eShaderReadOnlyOptimal };
        if (options.debug_texture == "Normal" || options.debug_texture == "Metallic")
            descriptor = { 0u, 0u, samplers.nearestClamp, images.normalMetallic.view, vk::ImageLayout::eShaderReadOnlyOptimal };
        if (options.debug_texture == "Depth")
            descriptor = { 0u, 0u, samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal };
        if (options.debug_texture == "Shadow" && options.shadow_quality != 0u)
            descriptor = { 0u, 0u, samplers.nearestClamp, images.shadowMiddle.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal };
        if (options.debug_texture == "SSAO" && options.ssao_quality != 0u)
            descriptor = { 0u, 0u, samplers.nearestClamp, images.ssaoBlur.view, vk::ImageLayout::eShaderReadOnlyOptimal };

        sq::vk_update_descriptor_set(ctx, sets.composite, descriptor);

        ctx.set_debug_object_name(pipelines.composite, "Renderer.Pipeline.Composite");
    }
}

//============================================================================//

void Renderer::impl_destroy_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.lighting);
    ctx.device.destroy(pipelineLayouts.lighting);

    ctx.device.destroy(pipelines.skybox);
    ctx.device.destroy(pipelines.lighting);
    ctx.device.destroy(pipelines.composite);

    ctx.device.free(ctx.descriptorPool, {sets.lighting.front, sets.lighting.back});
}

//============================================================================//

void Renderer::impl_create_shadow_stuff()
{
    if (options.shadow_quality == 0u) return;
    mNeedDestroyShadow = true;

    const auto& ctx = sq::VulkanContext::get();
    const uint shadowSize = SHADOW_MAP_BASE_SIZE * std::min(uint(options.shadow_quality), 2u);

    // create images and samplers
    {
        images.shadowFront.initialise_2D (
            ctx, vk::Format::eD16Unorm, Vec2U(shadowSize), 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eInputAttachment, false, {}, vk::ImageAspectFlagBits::eDepth
        );

        images.shadowBack.initialise_2D (
            ctx, vk::Format::eD16Unorm, Vec2U(shadowSize), 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eInputAttachment, false, {}, vk::ImageAspectFlagBits::eDepth
        );

        images.shadowMiddle.initialise_2D (
            ctx, vk::Format::eD16Unorm, Vec2U(shadowSize), 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eDepth
        );

        samplers.depthCompare = ctx.create_sampler (
            vk::Filter::eLinear, vk::Filter::eLinear, {},
            vk::SamplerAddressMode::eClampToBorder, vk::SamplerAddressMode::eClampToBorder, {},
            0.f, 0u, 0u, false, true, vk::BorderColor::eFloatOpaqueWhite
        );

        ctx.set_debug_object_name(images.shadowFront.image, "Renderer.Image.ShadowFront");
        ctx.set_debug_object_name(images.shadowFront.view, "Renderer.ImageView.ShadowFront");
        ctx.set_debug_object_name(images.shadowBack.image, "Renderer.Image.ShadowBack");
        ctx.set_debug_object_name(images.shadowBack.view, "Renderer.ImageView.ShadowBack");
        ctx.set_debug_object_name(images.shadowMiddle.image, "Renderer.Image.ShadowMiddle");
        ctx.set_debug_object_name(images.shadowMiddle.view, "Renderer.ImageView.ShadowMiddle");

        sq::vk_update_descriptor_set (
            ctx, sets.shadowMiddle,
            sq::DescriptorInputAttachment(0u, 0u, images.shadowFront.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal),
            sq::DescriptorInputAttachment(1u, 0u, images.shadowBack.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
        );
    }

    // create render passes and framebuffers
    {
        const auto attachments = std::array {
            vk::AttachmentDescription {
                {}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            },
            vk::AttachmentDescription {
                {}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            },
            vk::AttachmentDescription {
                {}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            }
        };

        const auto referenceFront = vk::AttachmentReference { 0u, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        const auto referenceBack = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilAttachmentOptimal };
        const auto referenceFrontInput = vk::AttachmentReference { 0u, vk::ImageLayout::eDepthStencilReadOnlyOptimal };
        const auto referenceBackInput = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilReadOnlyOptimal };
        const auto referenceMiddle = vk::AttachmentReference { 2u, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        const auto inputReferences = std::array { referenceFrontInput, referenceBackInput };

        const auto subpasses = std::array {
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, nullptr, nullptr, nullptr, &referenceFront, nullptr
            },
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, nullptr, nullptr, nullptr, &referenceBack, nullptr
            },
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, inputReferences, nullptr, nullptr, &referenceMiddle, nullptr
            }
        };

        const auto dependencies = std::array {
            vk::SubpassDependency {
                0u, 2u,
                vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            },
            vk::SubpassDependency {
                1u, 2u,
                vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            },
            vk::SubpassDependency {
                2u, VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            }
        };

        passes.shadow.initialise (
            ctx, attachments, subpasses, dependencies, Vec2U(shadowSize), 1u,
            { images.shadowFront.view, images.shadowBack.view, images.shadowMiddle.view }
        );

        ctx.set_debug_object_name(passes.shadow.pass, "Renderer.RenderPass.Shadow");
        ctx.set_debug_object_name(passes.shadow.framebuf, "Renderer.Framebuffer.Shadow");
    }

    // create shadow middle pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/shadow/Middle.frag.spv"
        );

        pipelines.shadowMiddle = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.shadowMiddle, passes.shadow.pass, 2u, shaderModules.stages, {},
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
                {}, true, true, vk::CompareOp::eAlways, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, 0.f, float(shadowSize), float(shadowSize), 0.f, 1.f },
            vk::Rect2D { {0, 0}, {shadowSize, shadowSize} },
            nullptr, nullptr
        );

        ctx.set_debug_object_name(pipelines.shadowMiddle, "Renderer.Pipeline.ShadowMiddle");
    }

    *mPassConfigShadowFront = sq::PassConfig {
        passes.shadow.pass, 0u, vk::SampleCountFlagBits::e1, vk::StencilOpState(),
        Vec2U(shadowSize), pipelineLayouts.shadow,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };

    *mPassConfigShadowBack = sq::PassConfig {
        passes.shadow.pass, 1u, vk::SampleCountFlagBits::e1, vk::StencilOpState(),
        Vec2U(shadowSize), pipelineLayouts.shadow,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };
}

//============================================================================//

void Renderer::impl_destroy_shadow_stuff()
{
    if (mNeedDestroyShadow == false) return;
    mNeedDestroyShadow = false;

    const auto& ctx = sq::VulkanContext::get();

    images.shadowFront.destroy(ctx);
    images.shadowBack.destroy(ctx);
    images.shadowMiddle.destroy(ctx);

    ctx.device.destroy(samplers.depthCompare);

    passes.shadow.destroy(ctx);

    ctx.device.destroy(pipelines.shadowMiddle);

    *mPassConfigShadowFront = sq::PassConfig();
    *mPassConfigShadowBack = sq::PassConfig();
}

//============================================================================//

void Renderer::impl_create_depth_mipmap_stuff()
{
    const auto& ctx = sq::VulkanContext::get();

    Vec2U sourceSize = window.get_size() / 2u * 2u;
    const uint numDepthMips = uint(std::log2(std::max(sourceSize.x, sourceSize.y))) - 1u;

    images.depthMips.initialise_2D (
        ctx, vk::Format::eD16Unorm, sourceSize / 2u, numDepthMips, vk::SampleCountFlagBits::e1,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, false,
        {}, vk::ImageAspectFlagBits::eDepth
    );

    ctx.set_debug_object_name(images.depthMips.image, "Renderer.Image.DepthMips");
    ctx.set_debug_object_name(images.depthMips.view, "Renderer.ImageView.DepthMips");

    samplers.depthMips = ctx.create_sampler (
        vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
        0.f, 0u, numDepthMips - 1u, false, false, {}
    );

    for (uint i = 0u; i < numDepthMips; ++i)
    {
        DepthMipGenStuff& stuff = mDepthMipGenStuff.emplace_back();

        stuff.dimensions = maths::max(sourceSize / 2u, Vec2U(1u, 1u));

        if (i == 0u) stuff.srcView = images.depthView;
        else stuff.srcView = mDepthMipGenStuff[i-1].destView;

        stuff.destView = ctx.create_image_view (
            images.depthMips.image, vk::ImageViewType::e2D, vk::Format::eD16Unorm,
            {}, vk::ImageAspectFlagBits::eDepth, i, 1u, 0u, 1u
        );

        // create depth mip render pass and framebuffer
        {
            const auto attachment = vk::AttachmentDescription {
                {}, vk::Format::eD16Unorm, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
            };

            const auto reference = vk::AttachmentReference { 0u, vk::ImageLayout::eDepthStencilAttachmentOptimal };

            const auto subpass = vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, nullptr, nullptr, nullptr, &reference, nullptr
            };

            const auto dependency = vk::SubpassDependency {
                0u, VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            };

            stuff.pass.initialise(ctx, attachment, subpass, dependency, stuff.dimensions, 1u, stuff.destView);
        }

        // create depth mip pipeline
        {
            const auto specialise = sq::SpecialisationInfo (
                int(sourceSize.x) & 1, int(sourceSize.y) & 1 // extra row, extra column
            );

            const auto shaderModules = sq::ShaderModules (
                ctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/DepthMips.frag.spv", &specialise.info
            );

            stuff.pipeline = sq::vk_create_graphics_pipeline (
                ctx, pipelineLayouts.depthMipGen, stuff.pass.pass, 0u, shaderModules.stages, {},
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
                    {}, true, true, vk::CompareOp::eAlways, false, false, {}, {}, 0.f, 0.f
                },
                vk::Viewport { 0.f, 0.f, float(stuff.dimensions.x), float(stuff.dimensions.y), 0.f, 1.f },
                vk::Rect2D { {0, 0}, {stuff.dimensions.x, stuff.dimensions.y} },
                nullptr, nullptr
            );

            stuff.descriptorSet = sq::vk_allocate_descriptor_set(ctx, setLayouts.depthMipGen);

            sq::vk_update_descriptor_set (
                ctx, stuff.descriptorSet,
                sq::DescriptorImageSampler(0u, 0u, samplers.nearestClamp, stuff.srcView, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
            );
        }

        sourceSize = stuff.dimensions;
    }
}

//============================================================================//

void Renderer::impl_destroy_depth_mipmap_stuff()
{
    const auto& ctx = sq::VulkanContext::get();

    images.depthMips.destroy(ctx);
    ctx.device.destroy(samplers.depthMips);

    for (DepthMipGenStuff& stuff : mDepthMipGenStuff)
    {
        stuff.pass.destroy(ctx);
        ctx.device.destroy(stuff.destView);
        ctx.device.destroy(stuff.pipeline);
        ctx.device.free(ctx.descriptorPool, stuff.descriptorSet);
    }

    mDepthMipGenStuff.clear();
}

//============================================================================//

void Renderer::impl_create_ssao_stuff()
{
    if (options.ssao_quality == 0u) return;
    mNeedDestroySSAO = true;

    const auto& ctx = sq::VulkanContext::get();
    const Vec2U halfSize = window.get_size() / 2u;

    // create ssao images
    {
        images.ssao.initialise_2D (
            ctx, vk::Format::eR16Sfloat, halfSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eColor
        );

        images.ssaoBlur.initialise_2D (
            ctx, vk::Format::eR8Unorm, halfSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eColor
        );

        ctx.set_debug_object_name(images.ssao.image, "Renderer.images.ssao");
        ctx.set_debug_object_name(images.ssao.view, "Renderer.images.ssaoView");
        ctx.set_debug_object_name(images.ssaoBlur.image, "Renderer.images.ssaoBlur");
        ctx.set_debug_object_name(images.ssaoBlur.view, "Renderer.images.ssaoBlurView");
    }

    // create ssao renderpass and framebuffer
    {
        const auto attachment = vk::AttachmentDescription {
            {}, vk::Format::eR16Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
        };

        const auto reference = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, reference, nullptr, nullptr, nullptr
        };

        const auto dependency = vk::SubpassDependency {
            0u, VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
            vk::DependencyFlagBits::eByRegion
        };

        passes.ssao.initialise(ctx, attachment, subpass, dependency, halfSize, 1u, images.ssao.view);

        ctx.set_debug_object_name(passes.ssao.pass, "Renderer.targets.ssaoRenderPass");
        ctx.set_debug_object_name(passes.ssao.framebuf, "Renderer.targets.ssaoFramebuffer");
    }

    // create ssao blur renderpass and framebuffer
    {
        const auto attachment = vk::AttachmentDescription {
            {}, vk::Format::eR8Unorm, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
        };

        const auto reference = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, reference, nullptr, nullptr, nullptr
        };

        const auto dependency = vk::SubpassDependency {
            0u, VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
            vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
            vk::DependencyFlagBits::eByRegion
        };

        passes.ssaoBlur.initialise(ctx, attachment, subpass, dependency, halfSize, 1u, images.ssaoBlur.view);

        ctx.set_debug_object_name(passes.ssaoBlur.pass, "Renderer.targets.ssaoBlurRenderPass");
        ctx.set_debug_object_name(passes.ssaoBlur.framebuf, "Renderer.targets.ssaoBlurFramebuffer");
    }

    // create ssao pipeline
    {
        const auto specialise = sq::SpecialisationInfo (
            1.f / float(halfSize.x), 1.f / float(halfSize.y), float(mDepthMipGenStuff.size()) * 0.5f
        );

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenFC.vert.spv", {},
            "shaders/ssao/GTAO/Quality{}.frag.spv"_format(options.ssao_quality),
            &specialise.info
        );

        pipelines.ssao = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.ssao, passes.ssao.pass, 0u, shaderModules.stages, {},
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
            vk::Viewport { 0.f, 0.f, float(halfSize.x), float(halfSize.y) },
            vk::Rect2D { {0, 0}, {halfSize.x, halfSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.ssao,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info()),
            sq::DescriptorImageSampler(1u, 0u, samplers.nearestClamp, images.normalMetallic.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(2u, 0u, samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal),
            sq::DescriptorImageSampler(3u, 0u, samplers.depthMips, images.depthMips.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
        );

        ctx.set_debug_object_name(pipelines.ssao, "Renderer.pipelines.ssao");
    }

    // create ssao blur pipeline
    {
        const auto specialise = sq::SpecialisationInfo (
            1.f / float(halfSize.x), 1.f / float(halfSize.y)
        );

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/ssao/BilateralBlur.frag.spv", &specialise.info
        );

        pipelines.ssaoBlur = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.ssaoBlur, passes.ssaoBlur.pass, 0u, shaderModules.stages, {},
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
            vk::Viewport { 0.f, 0.f, float(halfSize.x), float(halfSize.y) },
            vk::Rect2D { {0, 0}, {halfSize.x, halfSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.ssaoBlur,
            sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info()),
            sq::DescriptorImageSampler(1u, 0u, samplers.nearestClamp, images.ssao.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(2u, 0u, samplers.nearestClamp, images.depthMips.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
        );

        ctx.set_debug_object_name(pipelines.ssaoBlur, "Renderer.pipelines.ssaoBlur");
    }
}

//============================================================================//

void Renderer::impl_destroy_ssao_stuff()
{
    if (mNeedDestroySSAO == false) return;
    mNeedDestroySSAO = false;

    const auto& ctx = sq::VulkanContext::get();

    images.ssao.destroy(ctx);
    images.ssaoBlur.destroy(ctx);

    passes.ssao.destroy(ctx);
    passes.ssaoBlur.destroy(ctx);

    ctx.device.destroy(pipelines.ssao);
    ctx.device.destroy(pipelines.ssaoBlur);
}

//============================================================================//

void Renderer::refresh_options_destroy()
{
    impl_destroy_render_targets();
    impl_destroy_shadow_stuff();
    impl_destroy_depth_mipmap_stuff();
    impl_destroy_ssao_stuff();
    impl_destroy_pipelines();

    mDebugRenderer->refresh_options_destroy();
    mParticleRenderer->refresh_options_destroy();
}

void Renderer::refresh_options_create()
{
    impl_create_render_targets();
    impl_create_shadow_stuff();
    impl_create_depth_mipmap_stuff();
    impl_create_ssao_stuff();
    impl_create_pipelines();

    mDebugRenderer->refresh_options_create();
    mParticleRenderer->refresh_options_create();

    // won't exist when renderer is first created
    if (cubemaps.skybox.get_image())
        update_cubemap_descriptor_sets();
}

//============================================================================//

void Renderer::add_draw_call(const sq::DrawItem& item, const AnimPlayer& player)
{
    std::vector<DrawCall>* drawCalls = nullptr;

    if (item.pipeline->get_pass_config() == mPassConfigGbuffer)
        drawCalls = &mDrawCallsGbuffer;
    else if (item.pipeline->get_pass_config() == mPassConfigShadowFront)
        drawCalls = &mDrawCallsShadowFront;
    else if (item.pipeline->get_pass_config() == mPassConfigShadowBack)
        drawCalls = &mDrawCallsShadowBack;
    else if (item.pipeline->get_pass_config() == mPassConfigTransparent)
        drawCalls = &mDrawCallsTransparent;
    else SQEE_UNREACHABLE();

    drawCalls->emplace_back() = { &item, &player };
}

//============================================================================//

void Renderer::update_cubemap_descriptor_sets()
{
    const auto& ctx = sq::VulkanContext::get();

    sq::vk_update_descriptor_set_swapper (
        ctx, sets.skybox,
        sq::DescriptorImageSampler(1u, 0u, cubemaps.skybox.get_descriptor_info())
    );

    sq::vk_update_descriptor_set_swapper (
        ctx, sets.lighting,
        sq::DescriptorImageSampler(3u, 0u, cubemaps.irradiance.get_descriptor_info()),
        sq::DescriptorImageSampler(4u, 0u, cubemaps.radiance.get_descriptor_info())
    );

    sq::vk_update_descriptor_set_swapper (
        ctx, sets.particles,
        sq::DescriptorImageSampler(3u, 0u, cubemaps.irradiance.get_descriptor_info())
    );
}

//============================================================================//

void Renderer::swap_objects_buffers()
{
    mDrawCallsGbuffer.clear();
    mDrawCallsShadowFront.clear();
    mDrawCallsShadowBack.clear();
    mDrawCallsTransparent.clear();

    mNextMatrixIndex = 0u;
    ubos.matrices.swap_only();
}

Mat34F* Renderer::reserve_matrices(uint count, uint& index)
{
    index = mNextMatrixIndex;
    mNextMatrixIndex += count;
    return reinterpret_cast<Mat34F*>(ubos.matrices.map_only()) + index;
}

//============================================================================//

void Renderer::integrate_camera(float blend)
{
    mCamera->integrate(blend);

    auto& cameraBlock = *reinterpret_cast<CameraBlock*>(ubos.camera.swap_map());
    cameraBlock = mCamera->get_block();

    sets.gbuffer.swap();
    sets.shadow.swap();
    sets.ssao.swap();
    sets.ssaoBlur.swap();
    sets.skybox.swap();
    sets.lighting.swap();
    sets.transparent.swap();
    sets.particles.swap();
}

//============================================================================//

void Renderer::integrate_particles(float blend, const ParticleSystem& system)
{
    mParticleRenderer->integrate(blend, system);
}

//============================================================================//

void Renderer::integrate_debug(float blend, const World& world)
{
    mDebugRenderer->integrate(blend, world);
}

//============================================================================//

void Renderer::populate_command_buffer(vk::CommandBuffer cmdbuf)
{
    // query the previous frame timing, reset timestamps, and write start time
    {
        mTimestampQueryPool.swap();

        const auto& ctx = sq::VulkanContext::get();
        std::array<uint64_t, NUM_TIME_STAMPS + 1u> timestamps;

        const auto result = ctx.device.getQueryPoolResults (
            mTimestampQueryPool.front, 0u, NUM_TIME_STAMPS + 1u, 8u * (NUM_TIME_STAMPS + 1u),
            timestamps.data(), 8u, vk::QueryResultFlagBits::e64
        );

        if (result == vk::Result::eSuccess)
            for (uint i = 0u; i < NUM_TIME_STAMPS; ++i)
                mFrameTimings[i] = double(timestamps[i+1] - timestamps[i]) * double(ctx.limits.timestampPeriod) * 0.000001;

        cmdbuf.resetQueryPool(mTimestampQueryPool.front, 0u, NUM_TIME_STAMPS + 1u);
        cmdbuf.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, mTimestampQueryPool.front, 0u);
    }

    //--------------------------------------------------------//

    const auto submit_draw_calls = [cmdbuf](const sq::PassConfig& passConfig, const std::vector<DrawCall>& drawCalls)
    {
        const sq::Pipeline* prevPipeline = nullptr;
        const sq::Mesh* prevMesh = nullptr;

        std::array<std::byte, 128u> pushConstants;

        for (const DrawCall& call : drawCalls)
        {
            if (prevPipeline != &call.item->pipeline.get())
            {
                call.item->pipeline->bind(cmdbuf);
                prevPipeline = &call.item->pipeline.get();
            }

            if (prevMesh != &call.item->mesh.get())
            {
                call.item->mesh->bind_buffers(cmdbuf);
                prevMesh = &call.item->mesh.get();
            }

            call.item->compute_push_constants (
                call.player->blendSample,
                call.player->modelMatsIndex, call.player->normalMatsIndex,
                pushConstants
            );

            cmdbuf.pushConstants (
                passConfig.pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics,
                0u, pushConstants.size(), pushConstants.data()
            );

            call.item->mesh->draw(cmdbuf, call.item->subMesh);
        }
    };

    const auto clearValues = std::array {
        vk::ClearValue(vk::ClearColorValue().setFloat32({})),
        vk::ClearValue(vk::ClearColorValue().setFloat32({})),
        vk::ClearValue(vk::ClearDepthStencilValue(1.f))
    };

    const Vec2U halfSize = window.get_size() / 2u;
    const Vec2U renderSize = halfSize * 2u;
    const uint shadowSize = SHADOW_MAP_BASE_SIZE * std::min(uint(options.shadow_quality), 2u);

    //-- GBuffer Pass ----------------------------------------//

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.gbuffer, 0u, sets.gbuffer.front, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.gbuffer, 1u, caches.bindlessTextureSet, {});

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            passes.gbuffer.pass, passes.gbuffer.framebuf,
            vk::Rect2D({0, 0}, {renderSize.x, renderSize.y}),
            clearValues
        }, vk::SubpassContents::eInline
    );
    write_time_stamp(cmdbuf, TimeStamp::BeginGbuffer);

    submit_draw_calls(*mPassConfigGbuffer, mDrawCallsGbuffer);
    write_time_stamp(cmdbuf, TimeStamp::Opaque);

    cmdbuf.endRenderPass();
    write_time_stamp(cmdbuf, TimeStamp::EndGbuffer);

    //-- Generate Shadow Maps --------------------------------//

    if (options.shadow_quality != 0u)
    {
        const auto clearValues = std::array {
            vk::ClearValue(vk::ClearDepthStencilValue(1.f)),
            vk::ClearValue(vk::ClearDepthStencilValue(1.f))
        };

        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                passes.shadow.pass, passes.shadow.framebuf,
                vk::Rect2D({0, 0}, {shadowSize, shadowSize}),
                clearValues
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.shadow, 0u, sets.shadow.front, {});
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.shadow, 1u, caches.bindlessTextureSet, {});

        submit_draw_calls(*mPassConfigShadowFront, mDrawCallsShadowFront);
        cmdbuf.nextSubpass(vk::SubpassContents::eInline);

        submit_draw_calls(*mPassConfigShadowBack, mDrawCallsShadowBack);
        cmdbuf.nextSubpass(vk::SubpassContents::eInline);

        write_time_stamp(cmdbuf, TimeStamp::Shadows);

        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.shadowMiddle, 0u, sets.shadowMiddle, {});
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.shadowMiddle);
        cmdbuf.draw(3u, 1u, 0u, 0u);

        cmdbuf.endRenderPass();
    }
    else write_time_stamp(cmdbuf, TimeStamp::Shadows);

    write_time_stamp(cmdbuf, TimeStamp::ShadowAverage);

    //-- Generate Depth Mip Chain ----------------------------//

    for (const auto& stuff : mDepthMipGenStuff)
    {
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.depthMipGen, 0u, stuff.descriptorSet, {});

        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                stuff.pass.pass, stuff.pass.framebuf,
                vk::Rect2D({0, 0}, {stuff.dimensions.x, stuff.dimensions.y})
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, stuff.pipeline);
        cmdbuf.draw(3u, 1u, 0u, 0u);

        cmdbuf.endRenderPass();
    }

    write_time_stamp(cmdbuf, TimeStamp::DepthMipGen);

    //-- SSAO and SSAO Blur Passes ---------------------------//

    if (options.ssao_quality != 0u)
    {
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.ssao, 0u, sets.ssao.front, {});

        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                passes.ssao.pass, passes.ssao.framebuf,
                vk::Rect2D({0, 0}, {halfSize.x, halfSize.y})
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.ssao);
        cmdbuf.draw(3u, 1u, 0u, 0u);

        cmdbuf.endRenderPass();

        write_time_stamp(cmdbuf, TimeStamp::SSAO);

        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.ssaoBlur, 0u, sets.ssaoBlur.front, {});

        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                passes.ssaoBlur.pass, passes.ssaoBlur.framebuf,
                vk::Rect2D({0, 0}, {halfSize.x, halfSize.y})
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.ssaoBlur);
        cmdbuf.draw(3u, 1u, 0u, 0u);

        cmdbuf.endRenderPass();
    }
    else write_time_stamp(cmdbuf, TimeStamp::SSAO);

    write_time_stamp(cmdbuf, TimeStamp::BlurSSAO);

    //-- Deferred Shading Pass -------------------------------//

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            passes.hdr.pass, passes.hdr.framebuf,
            vk::Rect2D({0, 0}, {renderSize.x, renderSize.y})
        }, vk::SubpassContents::eInline
    );
    write_time_stamp(cmdbuf, TimeStamp::BeginHDR);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.skybox, 0u, sets.skybox.front, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);
    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::Skybox);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.lighting, 0u, sets.lighting.front, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.lighting);
    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::LightDefault);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.transparent, 0u, sets.transparent.front, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.transparent, 1u, caches.bindlessTextureSet, {});

    submit_draw_calls(*mPassConfigTransparent, mDrawCallsTransparent);
    write_time_stamp(cmdbuf, TimeStamp::Transparent);

    mParticleRenderer->populate_command_buffer(cmdbuf);
    write_time_stamp(cmdbuf, TimeStamp::Particles);

    // make sure that we are finished with the depth/stencil texture before the attachment store op
    sq::vk_pipeline_barrier_image_memory (
        cmdbuf, images.depthStencil.image, vk::DependencyFlagBits::eByRegion,
        vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::AccessFlagBits::eDepthStencilAttachmentRead, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal,
        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0u, 1u, 0u, 1u
    );

    cmdbuf.endRenderPass();
    write_time_stamp(cmdbuf, TimeStamp::EndHDR);
}

//============================================================================//

void Renderer::populate_final_pass(vk::CommandBuffer cmdbuf)
{
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.composite, 0u, sets.composite, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.composite);

    cmdbuf.pushConstants<decltype(tonemap)>(pipelineLayouts.composite, vk::ShaderStageFlagBits::eFragment, 0u, tonemap);

    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::Composite);

    mDebugRenderer->populate_command_buffer(cmdbuf);
    write_time_stamp(cmdbuf, TimeStamp::Debug);
}
