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

    mDebugRenderer = std::make_unique<DebugRenderer>(*this);
    mParticleRenderer = std::make_unique<ParticleRenderer>(*this);

    mSkyboxTexture.load_from_file_cube("assets/skybox/Space");

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
        sets.light = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.light);

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.light, 0u, 0u, vk::DescriptorType::eUniformBuffer,
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
    ctx.device.destroy(setLayouts.skybox);
    ctx.device.destroy(setLayouts.light);
    ctx.device.destroy(setLayouts.object);
    ctx.device.destroy(setLayouts.composite);

    ctx.device.free(ctx.descriptorPool, sets.camera.front);
    ctx.device.free(ctx.descriptorPool, sets.camera.back);
    ctx.device.free(ctx.descriptorPool, sets.skybox.front);
    ctx.device.free(ctx.descriptorPool, sets.skybox.back);
    ctx.device.free(ctx.descriptorPool, sets.light.front);
    ctx.device.free(ctx.descriptorPool, sets.light.back);
    ctx.device.free(ctx.descriptorPool, sets.composite);

    ctx.device.destroy(pipelineLayouts.skybox);
    ctx.device.destroy(pipelineLayouts.standard);
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
    setLayouts.skybox = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
            }
        }
    );
    setLayouts.light = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment
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
    sets.light = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.light);
    sets.composite = sq::vk_allocate_descriptor_set(ctx, setLayouts.composite);

    pipelineLayouts.skybox = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.skybox }, {});
    pipelineLayouts.standard = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.light }, {});
    pipelineLayouts.composite = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.composite }, {});
}

//============================================================================//

void Renderer::impl_create_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    const auto msaaMode = vk::SampleCountFlagBits(options.msaa_quality);
    const auto windowSize = window.get_size();

    //--------------------------------------------------------//

    // create images and samplers
    {
        if (msaaMode > vk::SampleCountFlagBits::e1)
        {
            std::tie(images.msColour, images.msColourMem, images.msColourView) = sq::vk_create_image_2D (
                ctx, vk::Format::eB8G8R8A8Srgb, windowSize, 1u, msaaMode, false,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
                false, {}, vk::ImageAspectFlagBits::eColor
            );
            ctx.set_debug_object_name(images.msColour, "renderer.images.msColour");
            ctx.set_debug_object_name(images.msColourView, "renderer.images.msColourView");
        }

        std::tie(images.resolveColour, images.resolveColourMem, images.resolveColourView) = sq::vk_create_image_2D (
            ctx, vk::Format::eB8G8R8A8Srgb, windowSize, 1u, vk::SampleCountFlagBits::e1, false,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );

        std::tie(images.depth, images.depthMem, images.depthView) = sq::vk_create_image_2D (
            ctx, vk::Format::eD32Sfloat, windowSize, 1u, msaaMode, false,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eInputAttachment,
            false, {}, vk::ImageAspectFlagBits::eDepth
        );

        samplers.resolveColour = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );

        ctx.set_debug_object_name(images.resolveColour, "renderer.images.resolveColour");
        ctx.set_debug_object_name(images.resolveColourView, "renderer.images.resolveColourView");
        ctx.set_debug_object_name(images.depth, "renderer.images.depth");
        ctx.set_debug_object_name(images.depthView, "renderer.images.depthView");
        ctx.set_debug_object_name(samplers.resolveColour, "renderer.samplers.resolveColour");
    }

    // create ms render pass
    {
        if (msaaMode > vk::SampleCountFlagBits::e1)
        {
            const auto attachments = std::array {
                vk::AttachmentDescription {
                    {}, vk::Format::eB8G8R8A8Srgb, msaaMode,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal
                },
                vk::AttachmentDescription {
                    {}, vk::Format::eD32Sfloat, msaaMode,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
                },
                vk::AttachmentDescription {
                    {}, vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
                }
            };

            const auto colourAttachment    = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };
            const auto depthAttachment     = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilAttachmentOptimal };
            const auto resolveAttachment   = vk::AttachmentReference { 2u, vk::ImageLayout::eColorAttachmentOptimal };
            const auto depthReadAttachment = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilReadOnlyOptimal };

            const auto subpasses = std::array {
                vk::SubpassDescription {
                    {}, vk::PipelineBindPoint::eGraphics, {}, colourAttachment, {}, &depthAttachment, {}
                },
                vk::SubpassDescription {
                    {}, vk::PipelineBindPoint::eGraphics, depthReadAttachment, colourAttachment, resolveAttachment, &depthReadAttachment, {}
                }
            };

            const auto dependencies = std::array {
                vk::SubpassDependency {
                    0u, 1u,
                    vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eDepthStencilAttachmentRead,
                    vk::DependencyFlagBits::eByRegion
                },
                vk::SubpassDependency {
                    1u, VK_SUBPASS_EXTERNAL,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
                    vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                    vk::DependencyFlagBits::eByRegion
                }
            };

            targets.mainRenderPass = ctx.device.createRenderPass (
                vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
            );
        }
        else // no multisample
        {
            const auto attachments = std::array {
                vk::AttachmentDescription {
                    {}, vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
                },
                vk::AttachmentDescription {
                    {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilReadOnlyOptimal
                }
            };

            const auto colourAttachment    = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };
            const auto depthAttachment     = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilAttachmentOptimal };
            const auto depthReadAttachment = vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilReadOnlyOptimal };

            const auto subpasses = std::array {
                vk::SubpassDescription {
                    {}, vk::PipelineBindPoint::eGraphics, {}, colourAttachment, {}, &depthAttachment, {}
                },
                vk::SubpassDescription {
                    {}, vk::PipelineBindPoint::eGraphics, depthReadAttachment, colourAttachment, {}, &depthReadAttachment, {}
                }
            };

            const auto dependencies = std::array {
                vk::SubpassDependency {
                    0u, 1u,
                    vk::PipelineStageFlagBits::eLateFragmentTests, vk::PipelineStageFlagBits::eEarlyFragmentTests,
                    vk::AccessFlagBits::eDepthStencilAttachmentWrite, vk::AccessFlagBits::eDepthStencilAttachmentRead,
                    vk::DependencyFlagBits::eByRegion
                },
                vk::SubpassDependency {
                    1u, VK_SUBPASS_EXTERNAL,
                    vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
                    vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                    vk::DependencyFlagBits::eByRegion
                }
            };

            targets.mainRenderPass = ctx.device.createRenderPass (
                vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
            );
        }
    }

    // create ms framebuffer
    {
        if (msaaMode > vk::SampleCountFlagBits::e1)
        {
            const auto attachments = std::array { images.msColourView, images.depthView, images.resolveColourView };

            targets.mainFramebuffer = ctx.device.createFramebuffer (
                vk::FramebufferCreateInfo { {}, targets.mainRenderPass, attachments, window.get_size().x, window.get_size().y, 1u }
            );
        }
        else // no multisample
        {
            const auto attachments = std::array { images.resolveColourView, images.depthView };

            targets.mainFramebuffer = ctx.device.createFramebuffer (
                vk::FramebufferCreateInfo { {}, targets.mainRenderPass, attachments, window.get_size().x, window.get_size().y, 1u }
            );
        }
    }

    ctx.set_debug_object_name(targets.mainRenderPass, "renderer.targets.mainRenderPass");
    ctx.set_debug_object_name(targets.mainFramebuffer, "renderer.targets.mainFramebuffer");

    caches.passConfigMap = {
        { "Opaque", { targets.mainRenderPass, 0u, msaaMode, window.get_size(), setLayouts.camera, setLayouts.light } },
        { "Transparent", { targets.mainRenderPass, 0u, msaaMode, window.get_size(), setLayouts.camera, setLayouts.light } }
    };
}

//============================================================================//

void Renderer::impl_destroy_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    if (images.msColourMem) ctx.device.destroy(images.msColourView);
    if (images.msColourMem) ctx.device.destroy(images.msColour);
    if (images.msColourMem) images.msColourMem.free();

    ctx.device.destroy(samplers.resolveColour);
    ctx.device.destroy(images.resolveColourView);
    ctx.device.destroy(images.resolveColour);
    images.resolveColourMem.free();

    ctx.device.destroy(images.depthView);
    ctx.device.destroy(images.depth);
    images.depthMem.free();

    ctx.device.destroy(targets.mainFramebuffer);
    ctx.device.destroy(targets.mainRenderPass);
}

//============================================================================//

void Renderer::impl_create_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    const auto msaaMode = vk::SampleCountFlagBits(options.msaa_quality);
    const auto windowSize = window.get_size();

    // skybox pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/Skybox.vert.spv", {}, "shaders/Skybox.frag.spv"
        );

        pipelines.skybox = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.skybox, targets.mainRenderPass, 0u, shaderModules.stages, {},
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
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
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );

        sq::vk_update_descriptor_set_swapper (
            ctx, sets.skybox, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
            mSkyboxTexture.get_descriptor_info(), mSkyboxTexture.get_descriptor_info()
        );
    }

    // composite pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/FullScreen.vert.spv", {}, "shaders/Composite.frag.spv"
        );

        pipelines.composite = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.composite, window.get_render_pass(), 0u, shaderModules.stages, {},
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eCounterClockwise, false, 0.f, 0.f, 0.f, 1.f
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

        sq::vk_update_descriptor_set (
            ctx, sets.composite, 0u, 0u, vk::DescriptorType::eCombinedImageSampler,
            vk::DescriptorImageInfo {
                samplers.resolveColour, images.resolveColourView, vk::ImageLayout::eShaderReadOnlyOptimal
            }
        );
    }

    ctx.set_debug_object_name(pipelines.skybox, "renderer.pipelines.skybox");
    ctx.set_debug_object_name(pipelines.composite, "renderer.pipelines.composite");
}

//============================================================================//

void Renderer::impl_destroy_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(pipelines.skybox);
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
        DrawItem& item = mDrawItems.emplace_back();

        if (def.condition.empty() == false)
            item.condition = conditions.at(def.condition);

        item.material = def.material;
        item.mesh = def.mesh;

        item.pass = def.pass;
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
    algo::erase_if(mDrawItems, predicate);
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

    sets.light.swap();
    auto& lightBlock = *reinterpret_cast<LightBlock*>(mLightUbo.swap_map());
    lightBlock.ambiColour = { 0.5f, 0.5f, 0.5f };
    lightBlock.skyColour = { 0.7f, 0.7f, 0.7f };
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
    //-- Begin the main render pass --------------------------//

    const auto clearValues = std::array {
        vk::ClearValue(vk::ClearColorValue().setFloat32({})),
        vk::ClearValue(vk::ClearDepthStencilValue(1.f))
    };

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            targets.mainRenderPass, targets.mainFramebuffer,
            vk::Rect2D({0, 0}, {window.get_size().x, window.get_size().y}),
            clearValues
        }, vk::SubpassContents::eInline
    );

    //-- Render the skybox -----------------------------------//

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.skybox, 0u, sets.camera.front, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.skybox, 1u, sets.skybox.front, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.skybox);

    cmdbuf.draw(3u, 1u, 0u, 0u);

    //--------------------------------------------------------//

    const auto render_items_for_draw_pass = [&](DrawPass pass)
    {
        const void* prevMtrl = nullptr;
        //const void* prevSet = nullptr;
        const void* prevMesh = nullptr;

        for (const DrawItem& item : mDrawItems)
        {
            // skip item if wrong pass or condition not satisfied
            if (item.pass != pass || (item.condition && *item.condition == item.invertCondition))
                continue;

            // TODO: seperate binding of pipeline and material
            if (prevMtrl != &item.material.get())
                item.material->bind(cmdbuf),
                    prevMtrl = &item.material.get();

            // TODO: set needs rebinding when pipeline changes
            //if (prevSet != item.descriptorSet)
            //    item.material->bind_final_descriptor_set(cmdbuf, item.descriptorSet->front),
            //        prevSet = item.descriptorSet;
            item.material->bind_final_descriptor_set(cmdbuf, item.descriptorSet->front);

            // TODO: does mesh need rebinding when pipeline changes?
            if (prevMesh != &item.mesh.get())
                item.mesh->bind_buffers(cmdbuf);
                    prevMesh = &item.mesh.get();

            item.mesh->draw(cmdbuf, item.subMesh);
        }
    };

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 0u, sets.camera.front, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 1u, sets.light.front, {});

    render_items_for_draw_pass(DrawPass::Opaque);
    render_items_for_draw_pass(DrawPass::Transparent);

    //--------------------------------------------------------//

    cmdbuf.nextSubpass(vk::SubpassContents::eInline);

    mParticleRenderer->populate_command_buffer(cmdbuf);

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
