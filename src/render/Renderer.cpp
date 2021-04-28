#include "render/Renderer.hpp"

#include "main/Options.hpp"

#include "render/Camera.hpp"
#include "render/DebugRender.hpp"
#include "render/ParticleRender.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Files.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/vk/VulkMaterial.hpp>
#include <sqee/vk/VulkWindow.hpp>
#include <sqee/vk/VulkMesh.hpp>

#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

Renderer::Renderer(const sq::VulkWindow& window, const Options& options, ResourceCaches& caches)
    : window(window), options(options), caches(caches)
{
//    mDebugRenderer = std::make_unique<DebugRenderer>(*this);
//    mParticleRenderer = std::make_unique<ParticleRenderer>(*this);

    impl_initialise_layouts();

    mSkyboxTexture.load_from_file_cube("assets/skybox");

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
                0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, nullptr
            }
        }
    );
    setLayouts.skybox = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment, nullptr
            }
        }
    );
    setLayouts.light = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eFragment, nullptr
            }
        }
    );
    setLayouts.object = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex, nullptr
            }
        }
    );
    setLayouts.composite = sq::vk_create_descriptor_set_layout (
        ctx, {}, {
            vk::DescriptorSetLayoutBinding {
                0u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment, nullptr
            }
        }
    );

    sets.camera = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.camera);
    sets.skybox = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.skybox);
    sets.light = sq::vk_allocate_descriptor_set_swapper(ctx, setLayouts.light);
    sets.composite = sq::vk_allocate_descriptor_set(ctx, setLayouts.composite);

    pipelineLayouts.skybox = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.skybox }, {});
    pipelineLayouts.standard = sq::vk_create_pipeline_layout(ctx, {}, { setLayouts.camera, setLayouts.light }, {});
    pipelineLayouts.composite = sq::vk_create_pipeline_layout (ctx, {}, { setLayouts.composite }, {});
}

//============================================================================//

void Renderer::impl_create_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    const auto msaaMode = vk::SampleCountFlagBits(options.msaa_quality);

    //--------------------------------------------------------//

    // create images and samplers
    {
        if (msaaMode > vk::SampleCountFlagBits::e1)
        {
            std::tie(images.msColour, images.msColourMem, images.msColourView) = sq::vk_create_image_2D (
                ctx, vk::Format::eB8G8R8A8Srgb, window.get_size(), msaaMode,
                false, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
                false, {}, vk::ImageAspectFlagBits::eColor
            );

            std::tie(images.msDepth, images.msDepthMem, images.msDepthView) = sq::vk_create_image_2D (
                ctx, vk::Format::eD32Sfloat, window.get_size(), msaaMode,
                false, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
                false, {}, vk::ImageAspectFlagBits::eDepth
            );
        }

        std::tie(images.resolveColour, images.resolveColourMem, images.resolveColourView) = sq::vk_create_image_2D (
            ctx, vk::Format::eB8G8R8A8Srgb, window.get_size(), vk::SampleCountFlagBits::e1,
            false, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eColor
        );

        std::tie(images.resolveDepth, images.resolveDepthMem, images.resolveDepthView) = sq::vk_create_image_2D (
            ctx, vk::Format::eD32Sfloat, window.get_size(), vk::SampleCountFlagBits::e1,
            false, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
            false, {}, vk::ImageAspectFlagBits::eDepth
        );

        samplers.resolveColour = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );

        samplers.resolveDepth = ctx.device.createSampler (
            vk::SamplerCreateInfo {
                {}, vk::Filter::eNearest, vk::Filter::eNearest, vk::SamplerMipmapMode::eNearest,
                vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
                0.f, false, 0.f, false, {}, 0.f, 0.f, {}, false
            }
        );
    }

    // create ms render pass
    {
        if (msaaMode > vk::SampleCountFlagBits::e1)
        {
            const auto attachments = std::array {
                vk::AttachmentDescription {
                    {}, vk::Format::eB8G8R8A8Srgb, msaaMode,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal
                },
                vk::AttachmentDescription {
                    {}, vk::Format::eD32Sfloat, msaaMode,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
                },
                vk::AttachmentDescription {
                    {}, vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
                },
                //vk::AttachmentDescription {
                //    {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                //    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                //    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                //    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
                //}
            };

            const auto colorAttachments = std::array {
                vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal }
            };

            const auto depthAttachment =
                vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilAttachmentOptimal };

            const auto resolveAttachments = std::array {
                vk::AttachmentReference { 2u, vk::ImageLayout::eColorAttachmentOptimal },
                //vk::AttachmentReference { 3u, vk::ImageLayout::eDepthAttachmentOptimal }
            };

            const auto subpasses = vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, {}, colorAttachments, resolveAttachments, &depthAttachment, {}
            };

            const auto dependencies = vk::SubpassDependency {
                0u, VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            };

            mMsRenderPass = ctx.device.createRenderPass (
                vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
            );
        }
        else // no multisample
        {
            const auto attachments = std::array {
                vk::AttachmentDescription {
                    {}, vk::Format::eB8G8R8A8Srgb, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
                },
                vk::AttachmentDescription {
                    {}, vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal
                }
            };

            const auto colorAttachments = std::array {
                vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal }
            };

            const auto depthAttachment =
                vk::AttachmentReference { 1u, vk::ImageLayout::eDepthStencilAttachmentOptimal };

            const auto subpasses = vk::SubpassDescription {
                {}, vk::PipelineBindPoint::eGraphics, nullptr, colorAttachments, nullptr, &depthAttachment, {}
            };

            const auto dependencies = vk::SubpassDependency {
                0u, VK_SUBPASS_EXTERNAL,
                vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead,
                vk::DependencyFlagBits::eByRegion
            };

            mMsRenderPass = ctx.device.createRenderPass (
                vk::RenderPassCreateInfo { {}, attachments, subpasses, dependencies }
            );
        }
    }

    // create ms framebuffer
    {
        if (msaaMode > vk::SampleCountFlagBits::e1)
        {
            const auto attachments = std::array {
                images.msColourView, images.msDepthView, images.resolveColourView//, images.resolveDepthView
            };

            mMsFramebuffer = ctx.device.createFramebuffer (
                vk::FramebufferCreateInfo {
                    {}, mMsRenderPass, attachments, window.get_size().x, window.get_size().y, 1u
                }
            );
        }
        else // no multisample
        {
            const auto attachments = std::array {
                images.resolveColourView, images.resolveDepthView
            };

            mMsFramebuffer = ctx.device.createFramebuffer (
                vk::FramebufferCreateInfo {
                    {}, mMsRenderPass, attachments, window.get_size().x, window.get_size().y, 1u
                }
            );
        }
    }

    caches.passConfigMap = {
        { "Opaque", { mMsRenderPass, msaaMode, window.get_size() } }
    };
}

//============================================================================//

void Renderer::impl_destroy_render_targets()
{
    const auto& ctx = sq::VulkanContext::get();

    if (images.msColourMem) ctx.device.destroy(images.msColourView);
    if (images.msColourMem) ctx.device.destroy(images.msColour);
    if (images.msColourMem) images.msColourMem.free();

    if (images.msDepthMem) ctx.device.destroy(images.msDepthView);
    if (images.msDepthMem) ctx.device.destroy(images.msDepth);
    if (images.msDepthMem) images.msDepthMem.free();

    ctx.device.destroy(samplers.resolveColour);
    ctx.device.destroy(images.resolveColourView);
    ctx.device.destroy(images.resolveColour);
    images.resolveColourMem.free();

    ctx.device.destroy(samplers.resolveDepth);
    ctx.device.destroy(images.resolveDepthView);
    ctx.device.destroy(images.resolveDepth);
    images.resolveDepthMem.free();

    ctx.device.destroy(mMsFramebuffer);
    ctx.device.destroy(mMsRenderPass);
}

//============================================================================//

void Renderer::impl_create_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    const auto msaaMode = vk::SampleCountFlagBits(options.msaa_quality);

    // skybox pipeline
    {
        const auto shaderModules = sq::ShaderModules (
            ctx, "shaders/Skybox.vert.spv", {}, "shaders/Skybox.frag.spv"
        );

        pipelines.skybox = sq::vk_create_graphics_pipeline (
            ctx, pipelineLayouts.skybox, mMsRenderPass, 0u, shaderModules.stages, {},
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
            vk::Viewport { 0.f, 0.f, float(window.get_size().x), float(window.get_size().y) },
            vk::Rect2D { {0, 0}, {window.get_size().x, window.get_size().y} },
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
            vk::Viewport { 0.f, 0.f, float(window.get_size().x), float(window.get_size().y) },
            vk::Rect2D { {0, 0}, {window.get_size().x, window.get_size().y} },
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
}

//============================================================================//

void Renderer::impl_destroy_pipelines()
{
    const auto& ctx = sq::VulkanContext::get();

    ctx.device.destroy(pipelines.skybox);
    ctx.device.destroy(pipelines.composite);
}

//============================================================================//

void Renderer::refresh_options()
{
    impl_destroy_render_targets();
    impl_create_render_targets();

    impl_destroy_pipelines();
    impl_create_pipelines();

    //-- Prepare Shader Options Header -----------------------//

    /*String headerStr = "// set of constants and defines added at runtime\n";

    headerStr += "const uint OPTION_WinWidth  = " + std::to_string(options.window_size.x) + ";\n";
    headerStr += "const uint OPTION_WinHeight = " + std::to_string(options.window_size.y) + ";\n";

    if (options.bloom_enable == true) headerStr += "#define OPTION_BLOOM_ENABLE\n";;
    if (options.ssao_quality != 0u)   headerStr += "#define OPTION_SSAO_ENABLE\n";
    if (options.ssao_quality >= 2u)   headerStr += "#define OPTION_SSAO_HIGH\n";

    headerStr += "// some handy shortcuts for comman use of this data\n"
                 "const float OPTION_Aspect = float(OPTION_WinWidth) / float(OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeFull = vec2(OPTION_WinWidth, OPTION_WinHeight);\n"
                 "const vec2 OPTION_WinSizeHalf = round(OPTION_WinSizeFull / 2.f);\n"
                 "const vec2 OPTION_WinSizeQter = round(OPTION_WinSizeFull / 4.f);\n";

    processor.import_header("runtime/Options", headerStr);

    //-- Allocate Target Textures ----------------------------//

    const uint msaaNum = std::max(4u * options.msaa_quality * options.msaa_quality, 1u);

    TEX_MsDepth = sq::TextureMulti();
    TEX_MsDepth.allocate_storage(sq::TexFormat::DEP24S8, options.window_size, msaaNum);

    TEX_MsColour = sq::TextureMulti();
    TEX_MsColour.allocate_storage(sq::TexFormat::RGB16_FP, options.window_size, msaaNum);

    TEX_Depth = sq::Texture2D();
    TEX_Depth.allocate_storage(sq::TexFormat::DEP24S8, options.window_size, false);

    TEX_Colour = sq::Texture2D();
    TEX_Colour.allocate_storage(sq::TexFormat::RGB16_FP, options.window_size, false);

    //-- Attach Textures to FrameBuffers ---------------------//

    FB_MsMain.attach(sq::FboAttach::DepthStencil, TEX_MsDepth);
    FB_MsMain.attach(sq::FboAttach::Colour0, TEX_MsColour);

    FB_Resolve.attach(sq::FboAttach::Depth, TEX_Depth);
    FB_Resolve.attach(sq::FboAttach::Colour0, TEX_Colour);

    //-- Load GLSL Shader Sources ----------------------------//

    processor.load_vertex(PROG_Particles, "shaders/particles/test_vs.glsl", {});
    processor.load_geometry(PROG_Particles, "shaders/particles/test_gs.glsl", {});
    processor.load_fragment(PROG_Particles, "shaders/particles/test_fs.glsl", {});

    processor.load_vertex(PROG_Lighting_Skybox, "shaders/stage/Skybox_vs.glsl", {});
    processor.load_fragment(PROG_Lighting_Skybox, "shaders/stage/Skybox_fs.glsl", {});

    processor.load_vertex(PROG_Composite, "shaders/FullScreen_vs.glsl", {});
    processor.load_fragment(PROG_Composite, "shaders/Composite_fs.glsl", {});

    //-- Link Shader Program Stages --------------------------//

    PROG_Particles.link_program_stages();

    PROG_Lighting_Skybox.link_program_stages();
    PROG_Composite.link_program_stages();

    //--------------------------------------------------------//

    mDebugRenderer->refresh_options();
    mParticleRenderer->refresh_options();*/
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

void Renderer::integrate(float blend)
{
    //-- Update the Camera -----------------------------------//

    mCameraUbo.swap();
    sets.camera.swap();

    mCamera->intergrate(blend);

    auto& cameraBlock = *reinterpret_cast<CameraBlock*>(mCameraUbo.map());
    cameraBlock = mCamera->get_block();

    //-- Update the Lighting ---------------------------------//

    mLightUbo.swap();
    sets.light.swap();

    auto& lightBlock = *reinterpret_cast<LightBlock*>(mLightUbo.map());
    lightBlock.ambiColour = { 0.5f, 0.5f, 0.5f };
    lightBlock.skyColour = { 0.7f, 0.7f, 0.7f };
    lightBlock.skyDirection = maths::normalize(Vec3F(0.f, -1.f, 0.5f));
    lightBlock.skyMatrix = Mat4F();
}

//============================================================================//

void Renderer::populate_command_buffer(vk::CommandBuffer cmdbuf)
{
    //-- Begin the main render pass --------------------------//

    const auto clearValues = std::array {
        vk::ClearValue(vk::ClearColorValue()), vk::ClearValue(vk::ClearDepthStencilValue(1.f))
    };

    cmdbuf.beginRenderPass (
        vk::RenderPassBeginInfo {
            mMsRenderPass, mMsFramebuffer,
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

            if (item.subMesh < 0) item.mesh->draw_complete(cmdbuf);
            else item.mesh->draw_submesh(cmdbuf, uint(item.subMesh));
        }
    };

    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 0u, sets.camera.front, {});
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.standard, 1u, sets.light.front, {});

    render_items_for_draw_pass(DrawPass::Opaque);

    //--------------------------------------------------------//

    cmdbuf.endRenderPass();

    /*context.set_ViewPort(options.window_size);

    context.bind_buffer(mCamera->get_ubo(), sq::BufTarget::Uniform, 0u);
    context.bind_buffer(mLightUbo, sq::BufTarget::Uniform, 1u);

    //-- Clear the FrameBuffer -------------------------------//

    context.bind_framebuffer(FB_MsMain);

    context.clear_depth_stencil_colour(1.0, 0x00, 0xFF, Vec4F(0.f));

    //-- Render the Skybox -----------------------------------//

    context.set_state(sq::BlendMode::Disable);
    context.set_state(sq::CullFace::Disable);
    context.set_state(sq::DepthTest::Disable);

    context.bind_texture(shit.TEX_Skybox, 0u);
    context.bind_program(PROG_Lighting_Skybox);

    context.bind_vertexarray_dummy();
    context.draw_arrays(sq::DrawPrimitive::TriangleStrip, 0u, 4u);

    //--------------------------------------------------------//

    const auto render_items_for_draw_pass = [&](DrawPass pass)
    {
        const sq::Material* prevMtrl = nullptr;
        const sq::FixedBuffer* prevUbo = nullptr;
        const sq::Mesh* prevMesh = nullptr;

        for (const DrawItem& item : mDrawItems)
        {
            // skip item if wrong pass or condition not satisfied
            if (item.pass != pass || (item.condition && *item.condition == item.invertCondition))
                continue;

            if (prevMtrl != &item.material.get())
                item.material->apply_to_context(context),
                    prevMtrl = &item.material.get();

            if (prevUbo != item.ubo)
                context.bind_buffer(*item.ubo, sq::BufTarget::Uniform, 2u),
                    prevUbo = item.ubo;

            if (prevMesh != &item.mesh.get())
                item.mesh->apply_to_context(context);
                    prevMesh = &item.mesh.get();

            if (item.subMesh < 0) item.mesh->draw_complete(context);
            else item.mesh->draw_submesh(context, uint(item.subMesh));
        }
    };

    //-- Render Opaque Pass ----------------------------------//

    context.bind_framebuffer(FB_MsMain);

    context.set_state(sq::DepthTest::Replace);
    context.set_depth_compare(sq::CompareFunc::LessEqual);

    render_items_for_draw_pass(DrawPass::Opaque);

    //-- Render Transparent Pass -----------------------------//

    context.bind_framebuffer(FB_MsMain);

    context.set_state(sq::DepthTest::Keep);
    context.set_state(sq::BlendMode::Alpha);
    context.set_state(sq::CullFace::Disable);
    context.set_depth_compare(sq::CompareFunc::LessEqual);

    render_items_for_draw_pass(DrawPass::Transparent);*/
}

//============================================================================//

void Renderer::populate_final_pass(vk::CommandBuffer cmdbuf)
{
    cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayouts.composite, 0u, sets.composite, {});
    cmdbuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelines.composite);

    cmdbuf.draw(3u, 1u, 0u, 0u);
}

//============================================================================//

void Renderer::render_particles(const ParticleSystem& system, float blend)
{
    mParticleRenderer->swap_sets();

    mParticleRenderer->integrate_set(blend, system);

    mParticleRenderer->render_particles();
}

//============================================================================//

void Renderer::resolve_multisample()
{
    //-- Resolve the Multi Sample Texture --------------------//

    //FB_MsMain.blit(FB_Resolve, options.window_size, sq::BlitMask::DepthColour);
}

//============================================================================//

void Renderer::finish_rendering()
{
    //-- Composite to the Default Framebuffer ----------------//

//    context.bind_framebuffer_default();

//    context.set_state(sq::BlendMode::Disable);
//    context.set_state(sq::CullFace::Disable);
//    context.set_state(sq::DepthTest::Disable);

//    context.bind_texture(TEX_Colour, 0u);
//    context.bind_program(PROG_Composite);

//    context.bind_vertexarray_dummy();
//    context.draw_arrays(sq::DrawPrimitive::TriangleStrip, 0u, 4u);
}
