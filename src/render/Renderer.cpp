#include "render/Renderer.hpp"

#include "main/Options.hpp"

#include "render/Camera.hpp"
#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"

#include <sqee/app/Window.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/objects/Material.hpp>
#include <sqee/objects/Mesh.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

Renderer::Renderer(const sq::Window& window, const Options& options, ResourceCaches& caches)
    : window(window), options(options), caches(caches)
{
    impl_initialise_layouts();

    mPassConfigOpaque = &caches.passConfigMap["Opaque"];
    mPassConfigTransparent = &caches.passConfigMap["Transparent"];

    mDebugRenderer = std::make_unique<DebugRenderer>(*this);
    mParticleRenderer = std::make_unique<ParticleRenderer>(*this);

    const auto& ctx = sq::VulkanContext::get();

    // initialise camera and environment uniform buffers
    {
        ubos.camera.initialise(sizeof(CameraBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        ubos.environment.initialise(sizeof(EnvironmentBlock), vk::BufferUsageFlagBits::eUniformBuffer);

        sets.camera = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.camera);
        sets.environment = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.environment);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.camera, sq::DescriptorUniformBuffer(0u, 0u, ubos.camera.get_descriptor_info())
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.environment, sq::DescriptorUniformBuffer(0u, 0u, ubos.environment.get_descriptor_info())
        );
    }

    // allocate other descriptor sets
    {
        sets.ssao = sq::vk_allocate_descriptor_set(ctx, setLayouts.ssao);
        sets.ssaoBlur = sq::vk_allocate_descriptor_set(ctx, setLayouts.ssaoBlur);
        sets.skybox = sq::vk_allocate_descriptor_set(ctx, setLayouts.skybox);
        sets.composite = sq::vk_allocate_descriptor_set(ctx, setLayouts.composite);
    }

    // create immutable samplers
    {
        samplers.nearestClamp = ctx.create_sampler (
            vk::Filter::eNearest, vk::Filter::eNearest, {},
            vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
            0.f, 0u, 0u, false, false
        );

        samplers.linearClamp = ctx.create_sampler (
            vk::Filter::eLinear, vk::Filter::eLinear, {},
            vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
            0.f, 0u, 0u, false, false
        );
    }

    mLutTexture.load_from_file_2D("assets/BrdfLut512");

    mTimestampQueryPool.front = ctx.device.createQueryPool({ {}, vk::QueryType::eTimestamp, NUM_TIME_STAMPS + 1u, {} });
    mTimestampQueryPool.back = ctx.device.createQueryPool({ {}, vk::QueryType::eTimestamp, NUM_TIME_STAMPS + 1u, {} });

    refresh_options_create();
}

//============================================================================//

Renderer::~Renderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.camera);
    ctx.device.destroy(setLayouts.environment);
    ctx.device.destroy(setLayouts.gbuffer);
    ctx.device.destroy(setLayouts.depthMipGen);
    ctx.device.destroy(setLayouts.ssao);
    ctx.device.destroy(setLayouts.ssaoBlur);
    ctx.device.destroy(setLayouts.skybox);
    ctx.device.destroy(setLayouts.object);
    ctx.device.destroy(setLayouts.composite);

    ctx.device.free(ctx.descriptorPool, sets.camera.front);
    ctx.device.free(ctx.descriptorPool, sets.camera.back);
    ctx.device.free(ctx.descriptorPool, sets.ssao);
    ctx.device.free(ctx.descriptorPool, sets.ssaoBlur);
    ctx.device.free(ctx.descriptorPool, sets.skybox);
    ctx.device.free(ctx.descriptorPool, sets.composite);

    ctx.device.destroy(pipelineLayouts.standard);
    ctx.device.destroy(pipelineLayouts.depthMipGen);
    ctx.device.destroy(pipelineLayouts.ssao);
    ctx.device.destroy(pipelineLayouts.ssaoBlur);
    ctx.device.destroy(pipelineLayouts.skybox);
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

    setLayouts.camera = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding {
            0u, vk::DescriptorType::eUniformBuffer, 1u,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment
        }
    });
    setLayouts.environment = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.gbuffer = ctx.create_descriptor_set_layout ({
    });
    setLayouts.depthMipGen = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.ssao = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.ssaoBlur = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.skybox = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });
    setLayouts.object = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex }
    });
    setLayouts.composite = ctx.create_descriptor_set_layout ({
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
    });

    pipelineLayouts.standard    = ctx.create_pipeline_layout({ setLayouts.camera }, {});
    pipelineLayouts.depthMipGen = ctx.create_pipeline_layout({ setLayouts.depthMipGen }, {});
    pipelineLayouts.ssao        = ctx.create_pipeline_layout({ setLayouts.camera, setLayouts.ssao }, {});
    pipelineLayouts.ssaoBlur    = ctx.create_pipeline_layout({ setLayouts.camera, setLayouts.ssaoBlur }, {});
    pipelineLayouts.skybox      = ctx.create_pipeline_layout({ setLayouts.camera, setLayouts.skybox }, {});
    pipelineLayouts.composite   = ctx.create_pipeline_layout({ setLayouts.composite }, {});
}

//============================================================================//

void Renderer::impl_create_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    const Vec2U renderSize = window.get_size() / 2u * 2u;
    const Vec2U halfSize = renderSize / 2u;

    // create images and samplers
    {
        const uint numDepthMips = uint(std::log2(std::max(renderSize.x, renderSize.y))) - 1u;

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

        images.depthMips.initialise_2D (
            ctx, vk::Format::eD16Unorm, halfSize, numDepthMips, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eDepth
        );

        images.colour.initialise_2D (
            ctx, vk::Format::eR16G16B16A16Sfloat, renderSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, false,
            {}, vk::ImageAspectFlagBits::eColor
        );

        samplers.depthMips = ctx.create_sampler (
            vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
            0.f, 0u, numDepthMips - 1u, false, false
        );

        ctx.set_debug_object_name(images.depthStencil.image, "Renderer.images.depthStencil");
        ctx.set_debug_object_name(images.depthStencil.view, "Renderer.images.depthStencilView");
        ctx.set_debug_object_name(images.depthView, "Renderer.images.depthView");
        ctx.set_debug_object_name(images.albedoRoughness.image, "Renderer.images.albedoRoughness");
        ctx.set_debug_object_name(images.albedoRoughness.view, "Renderer.images.albedoRoughnessView");
        ctx.set_debug_object_name(images.normalMetallic.image, "Renderer.images.normalMetallic");
        ctx.set_debug_object_name(images.normalMetallic.view, "Renderer.images.normalMetallicView");
        ctx.set_debug_object_name(images.depthMips.image, "Renderer.images.depthMips");
        ctx.set_debug_object_name(images.depthMips.view, "Renderer.images.depthMipsView");
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

        ctx.set_debug_object_name(passes.gbuffer.pass, "Renderer.targets.gbufferRenderPass");
        ctx.set_debug_object_name(passes.gbuffer.framebuf, "Renderer.targets.gbufferFramebuffer");
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

        ctx.set_debug_object_name(passes.hdr.pass, "Renderer.targets.hdrRenderPass");
        ctx.set_debug_object_name(passes.hdr.framebuf, "Renderer.targets.hdrFramebuffer");
    }

    *mPassConfigOpaque = sq::PassConfig {
        passes.gbuffer.pass, 0u, vk::SampleCountFlagBits::e1,
        vk::StencilOpState { {}, vk::StencilOp::eReplace, {}, vk::CompareOp::eAlways, 0, 0b1, 0b1 },
        window.get_size(), setLayouts.camera, setLayouts.gbuffer,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };

    *mPassConfigTransparent = sq::PassConfig {
        passes.hdr.pass, 0u, vk::SampleCountFlagBits::e1, vk::StencilOpState(),
        window.get_size(), setLayouts.camera, setLayouts.gbuffer,
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
    images.depthMips.destroy(ctx);
    images.colour.destroy(ctx);

    ctx.device.destroy(images.depthView);
    ctx.device.destroy(samplers.depthMips);

    passes.gbuffer.destroy(ctx);
    passes.hdr.destroy(ctx);
}

//============================================================================//

void Renderer::impl_create_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    const Vec2U renderSize = window.get_size() / 2u * 2u;

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
        setLayouts.lightDefault = options.ssao_quality == 0u ?
            ctx.create_descriptor_set_layout ({
                vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 3u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
            }) :
            ctx.create_descriptor_set_layout ({
                vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 3u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 4u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment },
                vk::DescriptorSetLayoutBinding { 5u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment }
            });

        pipelineLayouts.lightDefault = ctx.create_pipeline_layout({ setLayouts.camera, setLayouts.environment, setLayouts.lightDefault }, {});

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenTC.vert.spv", {},
            options.ssao_quality == 0u ? "shaders/lights/Default/NoSSAO.frag.spv" : "shaders/lights/Default/WithSSAO.frag.spv"
        );

        pipelines.lightDefault = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.lightDefault, passes.hdr.pass, 0u, shaderModules.stages, {},
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

        sets.lightDefault = ctx.allocate_descriptor_set(ctx.descriptorPool, setLayouts.lightDefault);

        sq::vk_update_descriptor_set (
            ctx, sets.lightDefault,
            sq::DescriptorImageSampler(0u, 0u, mLutTexture.get_descriptor_info()),
            sq::DescriptorImageSampler(1u, 0u, samplers.nearestClamp, images.albedoRoughness.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(2u, 0u, samplers.nearestClamp, images.normalMetallic.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(3u, 0u, samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
        );

        if (options.ssao_quality != 0u)
        {
            sq::vk_update_descriptor_set (
                ctx, sets.lightDefault,
                sq::DescriptorImageSampler(4u, 0u, samplers.linearClamp, images.ssaoBlur.view, vk::ImageLayout::eShaderReadOnlyOptimal),
                sq::DescriptorImageSampler(5u, 0u, samplers.nearestClamp, images.depthMips.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
            );
        }

        ctx.set_debug_object_name(pipelines.lightDefault, "Renderer.pipelines.lightDefault");
    }

    // composite pipeline
    {
        const auto specialise = sq::SpecialisationInfo (
            options.debug_texture == "NoToneMap" || options.debug_texture == "Albedo" ? 1 :
            options.debug_texture == "Roughness" || options.debug_texture == "Metallic" ? 2 :
            options.debug_texture == "Depth" ? 3 :
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
        if (options.debug_texture == "SSAO" && options.ssao_quality != 0u)
            descriptor = { 0u, 0u, samplers.nearestClamp, images.ssaoBlur.view, vk::ImageLayout::eShaderReadOnlyOptimal };

        sq::vk_update_descriptor_set(ctx, sets.composite, descriptor);

        ctx.set_debug_object_name(pipelines.composite, "Renderer.pipelines.composite");
    }
}

//============================================================================//

void Renderer::impl_destroy_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.lightDefault);
    ctx.device.destroy(pipelineLayouts.lightDefault);

    ctx.device.destroy(pipelines.skybox);
    ctx.device.destroy(pipelines.lightDefault);
    ctx.device.destroy(pipelines.composite);

    ctx.device.free(ctx.descriptorPool, sets.lightDefault);
}

//============================================================================//

void Renderer::impl_create_depth_mip_gen_stuff()
{
    const auto& ctx = sq::VulkanContext::get();

    Vec2U sourceSize = window.get_size() / 2u * 2u;
    const uint numDepthMips = uint(std::log2(std::max(sourceSize.x, sourceSize.y))) - 1u;

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

void Renderer::impl_destroy_depth_mip_gen_stuff()
{
    const auto& ctx = sq::VulkanContext::get();

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
            1.f / float(halfSize.x), 1.f / float(halfSize.y), int(mDepthMipGenStuff.size()) / 2
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

        sq::vk_update_descriptor_set (
            ctx, sets.ssao,
            sq::DescriptorImageSampler(0u, 0u, samplers.nearestClamp, images.normalMetallic.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(1u, 0u, samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal),
            sq::DescriptorImageSampler(2u, 0u, samplers.depthMips, images.depthMips.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
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

        sq::vk_update_descriptor_set (
            ctx, sets.ssaoBlur,
            sq::DescriptorImageSampler(0u, 0u, samplers.nearestClamp, images.ssao.view, vk::ImageLayout::eShaderReadOnlyOptimal),
            sq::DescriptorImageSampler(1u, 0u, samplers.nearestClamp, images.depthMips.view, vk::ImageLayout::eDepthStencilReadOnlyOptimal)
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
    impl_destroy_depth_mip_gen_stuff();
    impl_destroy_ssao_stuff();
    impl_destroy_pipelines();

    mDebugRenderer->refresh_options_destroy();
    mParticleRenderer->refresh_options_destroy();
}

void Renderer::refresh_options_create()
{
    impl_create_render_targets();
    impl_create_depth_mip_gen_stuff();
    impl_create_ssao_stuff();
    impl_create_pipelines();

    mDebugRenderer->refresh_options_create();
    mParticleRenderer->refresh_options_create();
}

//============================================================================//

void Renderer::set_camera(std::unique_ptr<Camera> camera)
{
    mCamera = std::move(camera);
}

//============================================================================//

int64_t Renderer::create_draw_items(const std::vector<DrawItemDef>& defs,
                                    const sq::Swapper<vk::DescriptorSet>& descriptorSet,
                                    std::map<TinyString, const bool*> conditions)
{
    const int64_t groupId = ++mCurrentGroupId;

    for (const DrawItemDef& def : defs)
    {
        std::vector<DrawItem>* drawItems = nullptr;

        if (def.material->get_pipeline().get_pass_config() == mPassConfigOpaque)
            drawItems = &mDrawItemsOpaque;
        if (def.material->get_pipeline().get_pass_config() == mPassConfigTransparent)
            drawItems = &mDrawItemsTransparent;

        DrawItem& item = drawItems->emplace_back();

        if (def.condition.empty() == false)
            item.condition = conditions.at(def.condition);

        item.material = def.material;
        item.mesh = def.mesh;

        item.invertCondition = def.invertCondition;
        item.subMesh = def.subMesh;

        item.descriptorSet = &descriptorSet;
        item.groupId = groupId;
    }

    // todo: sort draw items here

    return groupId;
}

void Renderer::delete_draw_items(int64_t groupId)
{
    const auto predicate = [groupId](DrawItem& item) { return item.groupId == groupId; };
    algo::erase_if(mDrawItemsOpaque, predicate);
    algo::erase_if(mDrawItemsTransparent, predicate);
}

//============================================================================//

void Renderer::integrate_camera(float blend)
{
    //-- Update the Camera -----------------------------------//

    mCamera->intergrate(blend);

    sets.camera.swap();
    auto& cameraBlock = *reinterpret_cast<CameraBlock*>(ubos.camera.swap_map());
    cameraBlock = mCamera->get_block();

    //-- Update the Environment ------------------------------//

    sets.environment.swap();
    auto& environmentBlock = *reinterpret_cast<EnvironmentBlock*>(ubos.environment.swap_map());
    environmentBlock.ambientColour = { 0.025f, 0.025f, 0.025f };
    environmentBlock.lightColour = { 3.0f, 3.0f, 3.0f };
    environmentBlock.lightDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));
    environmentBlock.lightMatrix = Mat4F();
}

//============================================================================//

void Renderer::integrate_particles(float blend, const ParticleSystem& system)
{
    mParticleRenderer->integrate(blend, system);
}

//============================================================================//

void Renderer::integrate_debug(float blend, const FightWorld& world)
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
        cmdbuf.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, mTimestampQueryPool.front, 0u);
    }

    //--------------------------------------------------------//

    const auto render_draw_items = [&](const std::vector<DrawItem>& drawItems)
    {
        const void *prevPipeline = nullptr, *prevMaterial = nullptr, *prevObject = nullptr, *prevMesh = nullptr;

        for (const DrawItem& item : drawItems)
        {
            // skip item if condition not satisfied
            if (item.condition && *item.condition == item.invertCondition)
                continue;

            if (prevPipeline != &item.material->get_pipeline())
            {
                item.material->get_pipeline().bind(cmdbuf);
                prevPipeline = &item.material->get_pipeline();
                prevObject = nullptr; // force rebinding
            }

            if (prevMaterial != &item.material.get())
            {
                item.material->bind_material_set(cmdbuf);
                prevMaterial = &item.material.get();
            }

            if (prevObject != item.descriptorSet)
            {
                item.material->bind_object_set(cmdbuf, item.descriptorSet->front);
                prevObject = item.descriptorSet;
            }

            if (prevMesh != &item.mesh.get())
            {
                item.mesh->bind_buffers(cmdbuf);
                prevMesh = &item.mesh.get();
            }

            item.mesh->draw(cmdbuf, item.subMesh);
        }
    };

    const auto clearValues = std::array {
        vk::ClearValue(vk::ClearColorValue().setFloat32({})),
        vk::ClearValue(vk::ClearColorValue().setFloat32({})),
        vk::ClearValue(vk::ClearDepthStencilValue(1.f))
    };

    const Vec2U halfSize = window.get_size() / 2u;
    const Vec2U renderSize = halfSize * 2u;

    //-- GBuffer Pass ----------------------------------------//

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 0u, sets.camera.front, {});

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            passes.gbuffer.pass, passes.gbuffer.framebuf,
            vk::Rect2D({0, 0}, {renderSize.x, renderSize.y}),
            clearValues
        }, vk::SubpassContents::eInline
    );
    write_time_stamp(cmdbuf, TimeStamp::BeginGbuffer);

    render_draw_items(mDrawItemsOpaque);
    write_time_stamp(cmdbuf, TimeStamp::Opaque);

    cmdbuf.endRenderPass();
    write_time_stamp(cmdbuf, TimeStamp::EndGbuffer);

    //-- Generate Depth Mip Chain ----------------------------//

    for (const auto& stuff : mDepthMipGenStuff)
    {
        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                stuff.pass.pass, stuff.pass.framebuf,
                vk::Rect2D({0, 0}, {stuff.dimensions.x, stuff.dimensions.y})
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.depthMipGen, 0u, stuff.descriptorSet, {});
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, stuff.pipeline);
        cmdbuf.draw(3u, 1u, 0u, 0u);

        cmdbuf.endRenderPass();
    }

    write_time_stamp(cmdbuf, TimeStamp::DepthMipGen);

    //--------------------------------------------------------//

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 0u, sets.camera.front, {});

    if (options.ssao_quality != 0u)
    {
        //-- SSAO Pass -------------------------------------------//

        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                passes.ssao.pass, passes.ssao.framebuf,
                vk::Rect2D({0, 0}, {halfSize.x, halfSize.y})
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.ssao, 1u, sets.ssao, {});
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.ssao);
        cmdbuf.draw(3u, 1u, 0u, 0u);

        cmdbuf.endRenderPass();

        write_time_stamp(cmdbuf, TimeStamp::SSAO);

        //-- SSAO Blur Pass --------------------------------------//

        cmdbuf.beginRenderPass (
            vk::RenderPassBeginInfo {
                passes.ssaoBlur.pass, passes.ssaoBlur.framebuf,
                vk::Rect2D({0, 0}, {halfSize.x, halfSize.y})
            }, vk::SubpassContents::eInline
        );

        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.ssaoBlur, 1u, sets.ssaoBlur, {});
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

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.skybox, 1u, sets.skybox, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);
    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::Skybox);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.lightDefault, 1u, sets.environment.front, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.lightDefault, 2u, sets.lightDefault, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.lightDefault);
    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::LightDefault);

    render_draw_items(mDrawItemsTransparent);
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

    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::Composite);

    mDebugRenderer->populate_command_buffer(cmdbuf);
    write_time_stamp(cmdbuf, TimeStamp::Debug);
}
