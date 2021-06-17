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

    // load brdf lookup texture
    {
        sq::Texture::Config config;
        config.format = vk::Format::eR16G16Unorm;
        config.wrapX = config.wrapY = config.wrapZ = vk::SamplerAddressMode::eClampToEdge;
        config.swizzle = vk::ComponentMapping();
        config.filter = sq::Texture::FilterMode::Linear;
        config.mipmaps = sq::Texture::MipmapsMode::Disable;
        config.size = Vec3U(512u, 512u, 1u);
        config.mipLevels = 1u;

        // todo: embed image data into the source
        mLutTexture.initialise_2D(config);
        mLutTexture.load_from_memory(sq::read_bytes_from_file("assets/BrdfLut512.bin").data(), 0u, 0u, config);
    }

    // todo: these need to be cached for the editor
    mSkyboxTexture.load_from_file_cube(options.editor_mode ? "assets/skybox/Space/small" : "assets/skybox/Space");
    mIrradianceTexture.load_from_file_cube("assets/skybox/Space/irradiance");
    mRadianceTexture.load_from_file_cube("assets/skybox/Space/radiance");

    const auto& ctx = sq::VulkanContext::get();

    // initialise camera ubo and update descriptor set
    {
        mCameraUbo.initialise(sizeof(CameraBlock), vk::BufferUsageFlagBits::eUniformBuffer);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.camera, 0u, 0u, vk::DescriptorType::eUniformBuffer,
            mCameraUbo.get_descriptor_info_front(), mCameraUbo.get_descriptor_info_back()
        );
    }

    // initialise default light ubo
    mLightUbo.initialise(sizeof(LightBlock), vk::BufferUsageFlagBits::eUniformBuffer);

    mTimestampQueryPool.front = ctx.device.createQueryPool({ {}, vk::QueryType::eTimestamp, NUM_TIME_STAMPS + 1u, {} });
    mTimestampQueryPool.back = ctx.device.createQueryPool({ {}, vk::QueryType::eTimestamp, NUM_TIME_STAMPS + 1u, {} });

    refresh_options_create();
}

//============================================================================//

Renderer::~Renderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.camera);
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

    refresh_options_destroy();
}

//============================================================================//

void Renderer::impl_initialise_layouts()
{
    const auto& ctx = sq::VulkanContext::get();

    using SSFB = vk::ShaderStageFlagBits;

    setLayouts.camera = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, SSFB::eVertex | SSFB::eGeometry | SSFB::eFragment }
    });
    setLayouts.gbuffer = sq::vk_create_descriptor_set_layout ( ctx, {}, {
    });
    setLayouts.depthMipGen = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
    });
    setLayouts.ssao = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
        vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
    });
    setLayouts.ssaoBlur = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
        vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
    });
    setLayouts.skybox = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
    });
    setLayouts.object = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, SSFB::eVertex }
    });
    setLayouts.composite = sq::vk_create_descriptor_set_layout ( ctx, {}, {
        vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
    });

    sets.camera = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.camera);
    sets.ssao = sq::vk_allocate_descriptor_set(ctx, setLayouts.ssao);
    sets.ssaoBlur = sq::vk_allocate_descriptor_set(ctx, setLayouts.ssaoBlur);
    sets.skybox = sq::vk_allocate_descriptor_set(ctx, setLayouts.skybox);
    sets.composite = sq::vk_allocate_descriptor_set(ctx, setLayouts.composite);

    pipelineLayouts.standard = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera }, {});
    pipelineLayouts.depthMipGen = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.depthMipGen }, {});
    pipelineLayouts.ssao = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.ssao }, {});
    pipelineLayouts.ssaoBlur = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.ssaoBlur }, {});
    pipelineLayouts.skybox = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.skybox }, {});
    pipelineLayouts.composite = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.composite }, {});
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

        std::tie(images.depthStencil, images.depthStencilMem, images.depthStencilView) = sq::vk_create_image_2D (
            ctx, vk::Format::eD24UnormS8Uint, renderSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil
        );
        ctx.set_debug_object_name(images.depthStencil, "Renderer.images.depthStencil");
        ctx.set_debug_object_name(images.depthStencilView, "Renderer.images.depthStencilView");

        images.depthView = ctx.device.createImageView (
            vk::ImageViewCreateInfo {
                {}, images.depthStencil, vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0u, 1u, 0u, 1u)
            }
        );
        ctx.set_debug_object_name(images.depthView, "Renderer.images.depthView");

        std::tie(images.albedoRoughness, images.albedoRoughnessMem, images.albedoRoughnessView) = sq::vk_create_image_2D (
            ctx, vk::Format::eB8G8R8A8Srgb, renderSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );
        ctx.set_debug_object_name(images.albedoRoughness, "Renderer.images.albedoRoughness");
        ctx.set_debug_object_name(images.albedoRoughnessView, "Renderer.images.albedoRoughnessView");

        std::tie(images.normalMetallic, images.normalMetallicMem, images.normalMetallicView) = sq::vk_create_image_2D (
            ctx, vk::Format::eR16G16B16A16Snorm, renderSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );
        ctx.set_debug_object_name(images.normalMetallic, "Renderer.images.normalMetallic");
        ctx.set_debug_object_name(images.normalMetallicView, "Renderer.images.normalMetallicView");

        std::tie(images.depthMips, images.depthMipsMem, images.depthMipsView) = sq::vk_create_image_2D (
            ctx, vk::Format::eD16Unorm, halfSize, numDepthMips, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eDepth
        );
        ctx.set_debug_object_name(images.depthMips, "Renderer.images.depthMips");
        ctx.set_debug_object_name(images.depthMipsView, "Renderer.images.depthMipsView");

        if (options.ssao_quality != 0u)
        {
            std::tie(images.ssao, images.ssaoMem, images.ssaoView) = sq::vk_create_image_2D (
                ctx, vk::Format::eR16Sfloat, halfSize, 1u, vk::SampleCountFlagBits::e1, false,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                false, {}, vk::ImageAspectFlagBits::eColor
            );
            ctx.set_debug_object_name(images.ssao, "Renderer.images.ssao");
            ctx.set_debug_object_name(images.ssaoView, "Renderer.images.ssaoView");

            std::tie(images.ssaoBlur, images.ssaoBlurMem, images.ssaoBlurView) = sq::vk_create_image_2D (
                ctx, vk::Format::eR8Unorm, halfSize, 1u, vk::SampleCountFlagBits::e1, false,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
                false, {}, vk::ImageAspectFlagBits::eColor
            );
            ctx.set_debug_object_name(images.ssaoBlur, "Renderer.images.ssaoBlur");
            ctx.set_debug_object_name(images.ssaoBlurView, "Renderer.images.ssaoBlurView");
        }

        std::tie(images.colour, images.colourMem, images.colourView) = sq::vk_create_image_2D (
            ctx, vk::Format::eR16G16B16A16Sfloat, renderSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );
        ctx.set_debug_object_name(images.colour, "Renderer.images.colour");
        ctx.set_debug_object_name(images.colourView, "Renderer.images.colourView");

        samplers.nearestClamp = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eNearest, vk::Filter::eNearest, {},
                vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );

        samplers.linearClamp = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eLinear, vk::Filter::eLinear, {},
                vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );

        samplers.depthMips = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eClampToEdge, vk::SamplerAddressMode::eClampToEdge, {},
                0.f, false, 0.f, false, {}, 0.f, float(numDepthMips), {}, false
            }
        );
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

        const auto subpasses = std::array {
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, nullptr, colourReferences, nullptr, &depthStencilReference, nullptr
            }
        };

        const auto dependencies = std::array {
            vk::SubpassDependency {
                0u, VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
                vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eEarlyFragmentTests,
                vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
                vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eDepthStencilAttachmentRead,
                vk::DependencyFlagBits::eByRegion
            }
        };

        targets.gbufferRenderPass = ctx.device.createRenderPass (
            vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
        );

        const auto fbAttachments = std::array { images.albedoRoughnessView, images.normalMetallicView, images.depthStencilView };

        targets.gbufferFramebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, targets.gbufferRenderPass, fbAttachments, renderSize.x, renderSize.y, 1u }
        );

        ctx.set_debug_object_name(targets.gbufferRenderPass, "Renderer.targets.gbufferRenderPass");
        ctx.set_debug_object_name(targets.gbufferFramebuffer, "Renderer.targets.gbufferFramebuffer");
    }

    // create ssao renderpass and framebuffer
    if (options.ssao_quality != 0u)
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

        targets.ssaoRenderPass = ctx.device.createRenderPass (
            vk::RenderPassCreateInfo { {}, attachment, subpass, dependency }
        );

        targets.ssaoFramebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, targets.ssaoRenderPass, images.ssaoView, halfSize.x, halfSize.y, 1u }
        );

        ctx.set_debug_object_name(targets.ssaoRenderPass, "Renderer.targets.ssaoRenderPass");
        ctx.set_debug_object_name(targets.ssaoFramebuffer, "Renderer.targets.ssaoFramebuffer");
    }

    // create ssao blur renderpass and framebuffer
    if (options.ssao_quality != 0u)
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

        targets.ssaoBlurRenderPass = ctx.device.createRenderPass (
            vk::RenderPassCreateInfo { {}, attachment, subpass, dependency }
        );

        targets.ssaoBlurFramebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, targets.ssaoBlurRenderPass, images.ssaoBlurView, halfSize.x, halfSize.y, 1u }
        );

        ctx.set_debug_object_name(targets.ssaoBlurRenderPass, "Renderer.targets.ssaoBlurRenderPass");
        ctx.set_debug_object_name(targets.ssaoBlurFramebuffer, "Renderer.targets.ssaoBlurFramebuffer");
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

        const auto subpasses = std::array {
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, nullptr, colourReference, nullptr, &depthStencilReference, nullptr
            }
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

        targets.hdrRenderPass = ctx.device.createRenderPass (
            vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
        );

        const auto fbAttachments = std::array { images.colourView, images.depthStencilView };

        targets.hdrFramebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, targets.hdrRenderPass, fbAttachments, renderSize.x, renderSize.y, 1u }
        );

        ctx.set_debug_object_name(targets.hdrRenderPass, "Renderer.targets.hdrRenderPass");
        ctx.set_debug_object_name(targets.hdrFramebuffer, "Renderer.targets.hdrFramebuffer");
    }

    *mPassConfigOpaque = sq::PassConfig {
        targets.gbufferRenderPass, 0u, vk::SampleCountFlagBits::e1,
        vk::StencilOpState { {}, vk::StencilOp::eReplace, {}, vk::CompareOp::eAlways, 0, 0b1, 0b1 },
        window.get_size(), setLayouts.camera, setLayouts.gbuffer,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };

    *mPassConfigTransparent = sq::PassConfig {
        targets.hdrRenderPass, 0u, vk::SampleCountFlagBits::e1, vk::StencilOpState(),
        window.get_size(), setLayouts.camera, setLayouts.gbuffer,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };
}

//============================================================================//

void Renderer::impl_destroy_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(images.depthView);
    ctx.device.destroy(images.depthStencilView);
    ctx.device.destroy(images.depthStencil);
    images.depthStencilMem.free();

    ctx.device.destroy(images.albedoRoughnessView);
    ctx.device.destroy(images.albedoRoughness);
    images.albedoRoughnessMem.free();

    ctx.device.destroy(images.normalMetallicView);
    ctx.device.destroy(images.normalMetallic);
    images.normalMetallicMem.free();

    ctx.device.destroy(images.depthMipsView);
    ctx.device.destroy(images.depthMips);
    images.depthMipsMem.free();

    ctx.device.destroy(images.colourView);
    ctx.device.destroy(images.colour);
    images.colourMem.free();

    ctx.device.destroy(samplers.nearestClamp);
    ctx.device.destroy(samplers.linearClamp);
    ctx.device.destroy(samplers.depthMips);

    ctx.device.destroy(targets.gbufferFramebuffer);
    ctx.device.destroy(targets.gbufferRenderPass);

    ctx.device.destroy(targets.hdrFramebuffer);
    ctx.device.destroy(targets.hdrRenderPass);

    if (mNeedDestroySSAO == true)
    {
        ctx.device.destroy(images.ssaoView);
        ctx.device.destroy(images.ssao);
        images.ssaoMem.free();

        ctx.device.destroy(images.ssaoBlurView);
        ctx.device.destroy(images.ssaoBlur);
        images.ssaoBlurMem.free();

        ctx.device.destroy(targets.ssaoFramebuffer);
        ctx.device.destroy(targets.ssaoRenderPass);

        ctx.device.destroy(targets.ssaoBlurFramebuffer);
        ctx.device.destroy(targets.ssaoBlurRenderPass);
    }
}

//============================================================================//

void Renderer::impl_create_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    const Vec2U halfSize = window.get_size() / 2u;
    const Vec2U renderSize = halfSize * 2u;

    using MapEntry = vk::SpecializationMapEntry;
    using SSFB = vk::ShaderStageFlagBits;

    if (options.ssao_quality != 0u) // ssao pipeline
    {
        const auto specMap = std::array { MapEntry(0u, 0u, 4u), MapEntry(1u, 4u, 4u), MapEntry(2u, 8u, 4u) };
        const auto specData = sq::Structure { 1.f / float(halfSize.x), 1.f / float(halfSize.y), int(mDepthMipGenStuff.size()) / 2 };
        const auto specialisation = vk::SpecializationInfo(3u, specMap.data(), 12u, &specData);

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenFC.vert.spv", {},
            "shaders/ssao/GTAO/Quality{}.frag.spv"_format(options.ssao_quality),
            &specialisation
        );

        pipelines.ssao = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.ssao, targets.ssaoRenderPass, 0u, shaderModules.stages, {},
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
            ctx, sets.ssao, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.normalMetallicView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set (
            ctx, sets.ssao, 1u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set (
            ctx, sets.ssao, 2u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.depthMips, images.depthMipsView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
        );

        ctx.set_debug_object_name(pipelines.ssao, "Renderer.pipelines.ssao");
    }

    if (options.ssao_quality != 0u) // ssao blur pipeline
    {
        const auto specMap = std::array { MapEntry(0u, 0u, 4u), MapEntry(1u, 4u, 4u) };
        const auto specData = std::array { 1.f / float(halfSize.x), 1.f / float(halfSize.y) };
        const auto specialisation = vk::SpecializationInfo(2u, specMap.data(), 8u, specData.data());

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/ssao/BilateralBlur.frag.spv", &specialisation
        );

        pipelines.ssaoBlur = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.ssaoBlur, targets.ssaoBlurRenderPass, 0u, shaderModules.stages, {},
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
            ctx, sets.ssaoBlur, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.ssaoView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set (
            ctx, sets.ssaoBlur, 1u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.depthMipsView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
        );

        ctx.set_debug_object_name(pipelines.ssaoBlur, "Renderer.pipelines.ssaoBlur");
    }

    // skybox pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/Skybox.vert.spv", {}, "shaders/Skybox.frag.spv"
        );

        pipelines.skybox = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.skybox, targets.hdrRenderPass, 0u, shaderModules.stages, {},
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

        sq::vk_update_descriptor_set (
            ctx, sets.skybox, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
            mSkyboxTexture.get_descriptor_info()
        );

        ctx.set_debug_object_name(pipelines.skybox, "Renderer.pipelines.skybox");
    }

    // default light pipeline
    {
        setLayouts.lightDefault = options.ssao_quality == 0u ?
            sq::vk_create_descriptor_set_layout ( ctx, {}, {
                vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 3u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 4u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 5u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 6u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
            }) :
            sq::vk_create_descriptor_set_layout ( ctx, {}, {
                vk::DescriptorSetLayoutBinding { 0u, vk::DescriptorType::eUniformBuffer, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 1u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 2u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 3u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 4u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 5u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 6u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 7u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment },
                vk::DescriptorSetLayoutBinding { 8u, vk::DescriptorType::eCombinedImageSampler, 1u, SSFB::eFragment }
            });

        pipelineLayouts.lightDefault = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.lightDefault }, {});

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenTC.vert.spv", {},
            options.ssao_quality == 0u ? "shaders/lights/Default/NoSSAO.frag.spv" : "shaders/lights/Default/WithSSAO.frag.spv"
        );

        pipelines.lightDefault = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.lightDefault, targets.hdrRenderPass, 0u, shaderModules.stages, {},
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

        sets.lightDefault = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.lightDefault);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 0u, 0u, vk::DescriptorType::eUniformBuffer,
            mLightUbo.get_descriptor_info_front(), mLightUbo.get_descriptor_info_back()
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 1u, 0u, vk::DescriptorType::eCombinedImageSampler,
            mLutTexture.get_descriptor_info(), mLutTexture.get_descriptor_info()
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 2u, 0u, vk::DescriptorType::eCombinedImageSampler,
            mIrradianceTexture.get_descriptor_info(), mIrradianceTexture.get_descriptor_info()
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 3u, 0u, vk::DescriptorType::eCombinedImageSampler,
            mRadianceTexture.get_descriptor_info(), mRadianceTexture.get_descriptor_info()
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 4u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.albedoRoughnessView, vk::ImageLayout::eShaderReadOnlyOptimal },
            vk::DescriptorImageInfo { samplers.nearestClamp, images.albedoRoughnessView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 5u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.normalMetallicView, vk::ImageLayout::eShaderReadOnlyOptimal },
            vk::DescriptorImageInfo { samplers.nearestClamp, images.normalMetallicView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 6u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal },
            vk::DescriptorImageInfo { samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
        );
        if (options.ssao_quality != 0u)
        {
            sq::vk_update_descriptor_set_swapper (
                ctx, sets.lightDefault, 7u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.linearClamp, images.ssaoBlurView, vk::ImageLayout::eShaderReadOnlyOptimal },
                vk::DescriptorImageInfo { samplers.linearClamp, images.ssaoBlurView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
            sq::vk_update_descriptor_set_swapper (
                ctx, sets.lightDefault, 8u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, images.depthMipsView, vk::ImageLayout::eDepthStencilReadOnlyOptimal },
                vk::DescriptorImageInfo { samplers.nearestClamp, images.depthMipsView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
            );
        }

        ctx.set_debug_object_name(pipelines.lightDefault, "Renderer.pipelines.lightDefault");
    }

    // composite pipeline
    {
        const int debugMode =
            options.debug_texture == "NoToneMap" || options.debug_texture == "Albedo" ? 1 :
            options.debug_texture == "Roughness" || options.debug_texture == "Metallic" ? 2 :
            options.debug_texture == "Depth" || options.debug_texture == "SSAO" ? 3 :
            options.debug_texture == "Normal" ? 4 : 0;

        const auto specMap = std::array { MapEntry(0u, 0u, 4u) };
        const auto specData = std::array { debugMode };
        const auto specialisation = vk::SpecializationInfo(1u, specMap.data(), 4u, specData.data());

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreenTC.vert.spv", {}, "shaders/Composite.frag.spv", &specialisation
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

        if (options.debug_texture == "Albedo" || options.debug_texture == "Roughness")
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, images.albedoRoughnessView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
        else if (options.debug_texture == "Normal" || options.debug_texture == "Metallic")
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, images.normalMetallicView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
        else if (options.debug_texture == "Depth")
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
            );
        else if (options.debug_texture == "SSAO" && options.ssao_quality != 0u)
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, images.ssaoBlurView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
        else
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, images.colourView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );

        ctx.set_debug_object_name(pipelines.composite, "Renderer.pipelines.composite");
    }
}

//============================================================================//

void Renderer::impl_destroy_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.lightDefault);
    ctx.device.free(ctx.descriptorPool, sets.lightDefault.front);
    ctx.device.free(ctx.descriptorPool, sets.lightDefault.back);
    ctx.device.destroy(pipelineLayouts.lightDefault);

    ctx.device.destroy(pipelines.skybox);
    ctx.device.destroy(pipelines.lightDefault);
    ctx.device.destroy(pipelines.composite);

    if (mNeedDestroySSAO == true)
    {
        ctx.device.destroy(pipelines.ssao);
        ctx.device.destroy(pipelines.ssaoBlur);
    }
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

        stuff.destView = ctx.device.createImageView (
            vk::ImageViewCreateInfo {
                {}, images.depthMips, vk::ImageViewType::e2D, vk::Format::eD16Unorm, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, i, 1u, 0u, 1u)
            }
        );

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

        stuff.renderPass = ctx.device.createRenderPass (
            vk::RenderPassCreateInfo { {}, attachment, subpass, dependency }
        );

        stuff.framebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, stuff.renderPass, stuff.destView, stuff.dimensions.x, stuff.dimensions.y, 1u }
        );

        // depth mips pipeline
        {
            const auto specMap = std::array { vk::SpecializationMapEntry(0u, 0u, 4u), vk::SpecializationMapEntry(1u, 4u, 4u) };
            const auto specData = std::array { int(sourceSize.x & 1u), int(sourceSize.y & 1u) };
            const auto specialisation = vk::SpecializationInfo(2u, specMap.data(), 8u, specData.data());

            const auto shaderModules = sq::ShaderModules (
                ctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/fullscreen/DepthMips.frag.spv", &specialisation
            );

            stuff.pipeline = sq::vk_create_graphics_pipeline (
                ctx, pipelineLayouts.depthMipGen, stuff.renderPass, 0u, shaderModules.stages, {},
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
                ctx, stuff.descriptorSet, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestClamp, stuff.srcView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
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
        ctx.device.destroy(stuff.destView);
        ctx.device.destroy(stuff.renderPass);
        ctx.device.destroy(stuff.framebuffer);
        ctx.device.destroy(stuff.pipeline);
        ctx.device.free(ctx.descriptorPool, stuff.descriptorSet);
    }

    mDepthMipGenStuff.clear();
}

//============================================================================//

void Renderer::refresh_options_destroy()
{
    impl_destroy_render_targets();
    impl_destroy_depth_mip_gen_stuff();
    impl_destroy_pipelines();

    mDebugRenderer->refresh_options_destroy();
    mParticleRenderer->refresh_options_destroy();
}

void Renderer::refresh_options_create()
{
    impl_create_render_targets();
    impl_create_depth_mip_gen_stuff();
    impl_create_pipelines();

    mDebugRenderer->refresh_options_create();
    mParticleRenderer->refresh_options_create();

    mNeedDestroySSAO = bool(options.ssao_quality);
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
    auto& cameraBlock = *reinterpret_cast<CameraBlock*>(mCameraUbo.swap_map());
    cameraBlock = mCamera->get_block();

    //-- Update the Lighting ---------------------------------//

    sets.lightDefault.swap();
    auto& lightBlock = *reinterpret_cast<LightBlock*>(mLightUbo.swap_map());
    lightBlock.ambientColour = { 0.05f, 0.05f, 0.05f };
    lightBlock.lightColour = { 3.0f, 3.0f, 3.0f };
    lightBlock.lightDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));
    lightBlock.lightMatrix = Mat4F();

    if (options.debug_toggle_1 == false) lightBlock.ambientColour.r = 0.f;
    else lightBlock.ambientColour.r = 1.f;
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
            targets.gbufferRenderPass, targets.gbufferFramebuffer,
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
                stuff.renderPass, stuff.framebuffer,
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
                targets.ssaoRenderPass, targets.ssaoFramebuffer,
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
                targets.ssaoBlurRenderPass, targets.ssaoBlurFramebuffer,
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
            targets.hdrRenderPass, targets.hdrFramebuffer,
            vk::Rect2D({0, 0}, {renderSize.x, renderSize.y})
        }, vk::SubpassContents::eInline
    );
    write_time_stamp(cmdbuf, TimeStamp::BeginHDR);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.skybox, 1u, sets.skybox, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);
    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::Skybox);

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.lightDefault, 1u, sets.lightDefault.front, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.lightDefault);
    cmdbuf.draw(3u, 1u, 0u, 0u);
    write_time_stamp(cmdbuf, TimeStamp::LightDefault);

    render_draw_items(mDrawItemsTransparent);
    write_time_stamp(cmdbuf, TimeStamp::Transparent);

    mParticleRenderer->populate_command_buffer(cmdbuf);
    write_time_stamp(cmdbuf, TimeStamp::Particles);

    // make sure that we are finished with the depth texture before the attachment store op
    cmdbuf.pipelineBarrier (
        vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eLateFragmentTests, vk::DependencyFlagBits::eByRegion, {}, {},
        vk::ImageMemoryBarrier {
            vk::AccessFlagBits::eDepthStencilAttachmentRead, vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eDepthStencilReadOnlyOptimal, vk::ImageLayout::eDepthStencilReadOnlyOptimal,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, images.depthStencil,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0u, 1u, 0u, 1u)
        }
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
