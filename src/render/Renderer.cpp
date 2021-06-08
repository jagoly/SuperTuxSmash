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

    // initialise camera ubo and descriptor set
    {
        mCameraUbo.initialise(sizeof(CameraBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        sets.camera = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.camera);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.camera, 0u, 0u, vk::DescriptorType::eUniformBuffer,
            mCameraUbo.get_descriptor_info_front(), mCameraUbo.get_descriptor_info_back()
        );
    }

    // initialise light ubo and descriptor set
    {
        mLightUbo.initialise(sizeof(LightBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        sets.lightDefault = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.lightDefault);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 0u, 0u, vk::DescriptorType::eUniformBuffer,
            mLightUbo.get_descriptor_info_front(), mLightUbo.get_descriptor_info_back()
        );
    }

    impl_create_render_targets();
    impl_create_pipelines();
}

//============================================================================//

Renderer::~Renderer()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(setLayouts.camera);
    ctx.device.destroy(setLayouts.gbuffer);
    ctx.device.destroy(setLayouts.skybox);
    ctx.device.destroy(setLayouts.lightDefault);
    ctx.device.destroy(setLayouts.object);
    ctx.device.destroy(setLayouts.composite);

    ctx.device.free(ctx.descriptorPool, sets.camera.front);
    ctx.device.free(ctx.descriptorPool, sets.camera.back);
    ctx.device.free(ctx.descriptorPool, sets.skybox.front);
    ctx.device.free(ctx.descriptorPool, sets.skybox.back);
    ctx.device.free(ctx.descriptorPool, sets.lightDefault.front);
    ctx.device.free(ctx.descriptorPool, sets.lightDefault.back);
    ctx.device.free(ctx.descriptorPool, sets.composite);

    ctx.device.destroy(pipelineLayouts.skybox);
    ctx.device.destroy(pipelineLayouts.standard);
    ctx.device.destroy(pipelineLayouts.lightDefault);
    ctx.device.destroy(pipelineLayouts.composite);

    impl_destroy_render_targets();
    impl_destroy_pipelines();
}

//============================================================================//

void Renderer::impl_initialise_layouts()
{
    const auto& ctx = sq::VulkanContext::get();

    setLayouts.camera = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eUniformBuffer, 1u,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment
            }
        }
    );
    setLayouts.gbuffer = sq::vk_create_descriptor_set_layout (
        ctx, {}, {}
    );
    setLayouts.skybox = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            }
        }
    );
    setLayouts.lightDefault = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment
            },
            vk::DescriptorSetLayoutBinding {
                1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            },
            vk::DescriptorSetLayoutBinding {
                2u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            },
            vk::DescriptorSetLayoutBinding {
                3u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            },
            vk::DescriptorSetLayoutBinding {
                4u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            },
            vk::DescriptorSetLayoutBinding {
                5u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            },
            vk::DescriptorSetLayoutBinding {
                6u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            }
        }
    );
    setLayouts.object = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex
            }
        }
    );
    setLayouts.composite = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            }
        }
    );

    sets.camera = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.camera);
    sets.skybox = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.skybox);
    sets.lightDefault = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.lightDefault);
    sets.composite = sq::vk_allocate_descriptor_set(ctx, setLayouts.composite);

    pipelineLayouts.standard = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera }, {});
    pipelineLayouts.skybox = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.skybox }, {});
    pipelineLayouts.lightDefault = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.lightDefault }, {});
    pipelineLayouts.composite = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.composite }, {});
}

//============================================================================//

void Renderer::impl_create_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    const auto windowSize = window.get_size();

    //--------------------------------------------------------//

    // create images and samplers
    {
        std::tie(images.depthStencil, images.depthStencilMem, images.depthStencilView) = sq::vk_create_image_2D (
            ctx, vk::Format::eD24UnormS8Uint, windowSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil
        );

        images.depthView = ctx.device.createImageView (
            vk::ImageViewCreateInfo {
                {}, images.depthStencil, vk::ImageViewType::e2D, vk::Format::eD24UnormS8Uint, {},
                vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0u, 1u, 0u, 1u)
            }
        );

        std::tie(images.albedoRoughness, images.albedoRoughnessMem, images.albedoRoughnessView) = sq::vk_create_image_2D (
            ctx, vk::Format::eB8G8R8A8Srgb, windowSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );

        std::tie(images.normalMetalic, images.normalMetalicMem, images.normalMetalicView) = sq::vk_create_image_2D (
            ctx, vk::Format::eR16G16B16A16Snorm, windowSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );

        std::tie(images.colour, images.colourMem, images.colourView) = sq::vk_create_image_2D (
            ctx, vk::Format::eR16G16B16A16Sfloat, windowSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );

        samplers.nearestRepeat = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );

        samplers.linearRepeat = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );

        ctx.set_debug_object_name(images.depthStencil, "Renderer.images.depthStencil");
        ctx.set_debug_object_name(images.depthStencilView, "Renderer.images.depthStencilView");
        ctx.set_debug_object_name(images.depthView, "Renderer.images.depthView");
        ctx.set_debug_object_name(images.albedoRoughness, "Renderer.images.albedoRoughness");
        ctx.set_debug_object_name(images.albedoRoughnessView, "Renderer.images.albedoRoughnessView");
        ctx.set_debug_object_name(images.normalMetalic, "Renderer.images.normalMetalic");
        ctx.set_debug_object_name(images.normalMetalicView, "Renderer.images.normalMetalicView");
        ctx.set_debug_object_name(images.colour, "Renderer.images.colour");
        ctx.set_debug_object_name(images.colourView, "Renderer.images.colourView");
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

        const auto albedoRoughAttachment = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };
        const auto normalMetalAttachment = vk::AttachmentReference { 1u, vk::ImageLayout::eColorAttachmentOptimal };
        const auto depthAttachment       = vk::AttachmentReference { 2u, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        const auto colourAttachments = std::array { albedoRoughAttachment, normalMetalAttachment };

        const auto subpasses = std::array {
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, {}, colourAttachments, {}, &depthAttachment, {}
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

        const auto fbAttachments = std::array { images.albedoRoughnessView, images.normalMetalicView, images.depthStencilView };

        targets.gbufferFramebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, targets.gbufferRenderPass, fbAttachments, window.get_size().x, window.get_size().y, 1u }
        );

        ctx.set_debug_object_name(targets.gbufferRenderPass, "Renderer.targets.gbufferRenderPass");
        ctx.set_debug_object_name(targets.gbufferFramebuffer, "Renderer.targets.gbufferFramebuffer");
    }

    // create lights renderpass and framebuffer
    {
        const auto attachments = std::array {
            vk::AttachmentDescription {
                {}, vk::Format::eR16G16B16A16Sfloat, vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
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

        const auto colourAttachment  = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };
        const auto stencilAttachment = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilReadOnlyOptimal };

        const auto subpasses = std::array {
            vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, {}, colourAttachment, {}, &stencilAttachment, {}
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

        targets.lightsRenderPass = ctx.device.createRenderPass (
            vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
        );

        const auto fbAttachments = std::array { images.colourView, images.depthStencilView };

        targets.lightsFramebuffer = ctx.device.createFramebuffer (
            vk::FramebufferCreateInfo { {}, targets.lightsRenderPass, fbAttachments, window.get_size().x, window.get_size().y, 1u }
        );

        ctx.set_debug_object_name(targets.lightsRenderPass, "Renderer.targets.lightsRenderPass");
        ctx.set_debug_object_name(targets.lightsFramebuffer, "Renderer.targets.lightsFramebuffer");
    }

    *mPassConfigOpaque = sq::PassConfig {
        targets.gbufferRenderPass, 0u, vk::SampleCountFlagBits::e1,
        vk::StencilOpState { {}, vk::StencilOp::eReplace, {}, vk::CompareOp::eAlways, 0, 0b1, 0b1 },
        window.get_size(), setLayouts.camera, setLayouts.gbuffer,
        sq::SpecialisationConstants(4u, int(options.debug_toggle_1), 5u, int(options.debug_toggle_2))
    };

    *mPassConfigTransparent = sq::PassConfig {
        targets.lightsRenderPass, 0u, vk::SampleCountFlagBits::e1, vk::StencilOpState(),
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

    ctx.device.destroy(images.normalMetalicView);
    ctx.device.destroy(images.normalMetalic);
    images.normalMetalicMem.free();

    ctx.device.destroy(images.colourView);
    ctx.device.destroy(images.colour);
    images.colourMem.free();

    ctx.device.destroy(samplers.nearestRepeat);
    ctx.device.destroy(samplers.linearRepeat);

    ctx.device.destroy(targets.gbufferFramebuffer);
    ctx.device.destroy(targets.gbufferRenderPass);

    ctx.device.destroy(targets.lightsFramebuffer);
    ctx.device.destroy(targets.lightsRenderPass);
}

//============================================================================//

void Renderer::impl_create_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();
    const auto windowSize = window.get_size();

    // skybox pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/Skybox.vert.spv", {}, "shaders/Skybox.frag.spv"
        );

        pipelines.skybox = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.skybox, targets.lightsRenderPass, 0u, shaderModules.stages, {},
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
            vk::Viewport { 0.f, 0.f, float(windowSize.x), float(windowSize.y) },
            vk::Rect2D { {0, 0}, {windowSize.x, windowSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.skybox, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
            mSkyboxTexture.get_descriptor_info(), mSkyboxTexture.get_descriptor_info()
        );
    }

    // default light pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreen.vert.spv", {}, "shaders/lights/Default.frag.spv"
        );

        pipelines.lightDefault = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.lightDefault, targets.lightsRenderPass, 0u, shaderModules.stages, {},
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
            vk::Viewport { 0.f, 0.f, float(windowSize.x), float(windowSize.y) },
            vk::Rect2D { {0, 0}, {windowSize.x, windowSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
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
            vk::DescriptorImageInfo { samplers.nearestRepeat, images.albedoRoughnessView, vk::ImageLayout::eShaderReadOnlyOptimal },
            vk::DescriptorImageInfo { samplers.nearestRepeat, images.albedoRoughnessView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 5u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestRepeat, images.normalMetalicView, vk::ImageLayout::eShaderReadOnlyOptimal },
            vk::DescriptorImageInfo { samplers.nearestRepeat, images.normalMetalicView, vk::ImageLayout::eShaderReadOnlyOptimal }
        );
        sq::vk_update_descriptor_set_swapper (
            ctx, sets.lightDefault, 6u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo { samplers.nearestRepeat, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal },
            vk::DescriptorImageInfo { samplers.nearestRepeat, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
        );
    }

    // composite pipeline
    {
        const auto specialisationMap = vk::SpecializationMapEntry(0u, 0u, sizeof(int));
        const auto specialisationData = int (
            options.debug_texture == "NoToneMap" || options.debug_texture == "Albedo" ? 1 :
            options.debug_texture == "Roughness" || options.debug_texture == "Metallic" ? 2 :
            options.debug_texture == "Depth" || options.debug_texture == "SSAO" ? 3 :
            options.debug_texture == "Normal" ? 4 : 0
        );
        const auto specialisation = vk::SpecializationInfo(1u, &specialisationMap, sizeof(int), &specialisationData);

        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreen.vert.spv", {}, "shaders/Composite.frag.spv", &specialisation
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
            vk::Viewport { 0.f, 0.f, float(windowSize.x), float(windowSize.y) },
            vk::Rect2D { {0, 0}, {windowSize.x, windowSize.y} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        if (options.debug_texture == "Albedo" || options.debug_texture == "Roughness")
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestRepeat, images.albedoRoughnessView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
        else if (options.debug_texture == "Normal" || options.debug_texture == "Metallic")
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestRepeat, images.normalMetalicView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
        else if (options.debug_texture == "Depth")
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestRepeat, images.depthView, vk::ImageLayout::eDepthStencilReadOnlyOptimal }
            );
        else
            sq::vk_update_descriptor_set (
                ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
                vk::DescriptorImageInfo { samplers.nearestRepeat, images.colourView, vk::ImageLayout::eShaderReadOnlyOptimal }
            );
    }

    ctx.set_debug_object_name(pipelines.skybox, "Renderer.pipelines.skybox");
    ctx.set_debug_object_name(pipelines.lightDefault, "Renderer.pipelines.lightDefault");
    ctx.set_debug_object_name(pipelines.composite, "Renderer.pipelines.composite");
}

//============================================================================//

void Renderer::impl_destroy_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(pipelines.skybox);
    ctx.device.destroy(pipelines.lightDefault);
    ctx.device.destroy(pipelines.composite);
}

//============================================================================//

void Renderer::refresh_options_destroy()
{
    impl_destroy_render_targets();
    impl_destroy_pipelines();

    mDebugRenderer->refresh_options_destroy();
    mParticleRenderer->refresh_options_destroy();
}

void Renderer::refresh_options_create()
{
    impl_create_render_targets();
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
    auto& cameraBlock = *reinterpret_cast<CameraBlock*>(mCameraUbo.swap_map());
    cameraBlock = mCamera->get_block();

    //-- Update the Lighting ---------------------------------//

    sets.lightDefault.swap();
    auto& lightBlock = *reinterpret_cast<LightBlock*>(mLightUbo.swap_map());
    lightBlock.ambiColour = { 0.1f, 0.1f, 0.1f };
    lightBlock.skyColour = { 3.0f, 3.0f, 3.0f };
    lightBlock.skyDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));
    lightBlock.skyMatrix = Mat4F();
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

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 0u, sets.camera.front, {});

    //-- GBuffer Pass ----------------------------------------//

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            targets.gbufferRenderPass, targets.gbufferFramebuffer,
            vk::Rect2D({0, 0}, {window.get_size().x, window.get_size().y}),
            clearValues
        }, vk::SubpassContents::eInline
    );

    render_draw_items(mDrawItemsOpaque);

    cmdbuf.endRenderPass();

    //-- Deferred Shading Pass -------------------------------//

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            targets.lightsRenderPass, targets.lightsFramebuffer,
            vk::Rect2D({0, 0}, {window.get_size().x, window.get_size().y}),
            clearValues.front()
        }, vk::SubpassContents::eInline
    );

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.skybox, 1u, sets.skybox.front, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);
    cmdbuf.draw(3u, 1u, 0u, 0u);

    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.lightDefault);
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.lightDefault, 1u, sets.lightDefault.front, {});
    cmdbuf.draw(3u, 1u, 0u, 0u);

    render_draw_items(mDrawItemsTransparent);

    mParticleRenderer->populate_command_buffer(cmdbuf);

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
}

//============================================================================//

void Renderer::populate_final_pass(vk::CommandBuffer cmdbuf)
{
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.composite, 0u, sets.composite, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.composite);

    cmdbuf.draw(3u, 1u, 0u, 0u);

    mDebugRenderer->populate_command_buffer(cmdbuf);
}
