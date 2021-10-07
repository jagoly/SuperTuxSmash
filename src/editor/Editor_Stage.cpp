#include "editor/EditorScene.hpp" // IWYU pragma: associated
#include "editor/Editor_Helpers.hpp"

#include "render/Renderer.hpp"

#include "game/Stage.hpp"

#include <sqee/app/GuiWidgets.hpp>
#include <sqee/maths/Colours.hpp>
#include <sqee/vk/Helpers.hpp>
#include <sqee/vk/VulkanContext.hpp>

using namespace sts;

//============================================================================//

void EditorScene::impl_show_widget_stage()
{
    if (mDoResetDockStage) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockStage = false;

    const ImPlus::ScopeWindow window = { "Stage", 0 };
    if (window.show == false) return;

    StageContext& ctx = *mActiveStageContext;

    //--------------------------------------------------------//

    const ImPlus::ScopeID ctxKeyIdScope = int(ctx.key);

    if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeItemWidth width = -100.f;

        auto& tonemap = ctx.renderer->tonemap;

        ImPlus::SliderValue("Exposure", tonemap.exposure, 0.25f, 4.f);
        ImPlus::SliderValue("Contrast", tonemap.contrast, 0.5f, 2.f);
        ImPlus::SliderValue("Black", tonemap.black, 0.5f, 2.f);
    }

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const ImPlus::ScopeItemWidth width = -100.f;

        ImPlus::InputVector("Colour", ctx.stage->mLightColour, 0, "%.2f");
        ImPlus::InputVector("Direction", ctx.stage->mLightDirection, 0, "%.2f");
    }
}

//============================================================================//

void EditorScene::impl_show_widget_cubemaps()
{
    if (mDoResetDockCubemaps) ImGui::SetNextWindowDockID(mDockRightId);
    mDoResetDockCubemaps = false;

    const ImPlus::ScopeWindow window = { "CubeMaps", 0 };
    if (window.show == false) return;

    StageContext& ctx = *mActiveStageContext;

    //--------------------------------------------------------//

    const ImPlus::ScopeID ctxKeyIdScope = int(ctx.key);

    const ImGuiStyle& style = ImGui::GetStyle();

    constexpr auto faceNames = std::array { "Right", "Left", "Down", "Up", "Forward", "Back" };
    const float size = std::floor((ImGui::GetWindowContentRegionWidth() - 5.f * style.ItemSpacing.x) / 6.f);

    // ordered so that you get a panorama for the first four faces
    const uint faceMap[] = { 4, 0, 5, 1, 2, 3 };

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader("Skybox"))
    {
        if (ImGui::Button("Save"))
        {
            ctx.renderer->cubemaps.skybox.save_as_compressed (
                sq::build_string("assets/", ctx.stage->get_skybox_path(), "/Sky.lz4"),
                vk::Format::eE5B9G9R9UfloatPack32, Vec3U(SKYBOX_SIZE, SKYBOX_SIZE, 6u), 1u
            );
        }

        for (uint face = 0u; face < 6u; ++face)
        {
            ImGui::Image(&ctx.skybox.descriptorSets[faceMap[face]], {size, size}, {0,1}, {1,0});
            ImPlus::HoverTooltip(faceNames[faceMap[face]]);
            if (face < 5u) ImGui::SameLine();
        }
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader(ctx.irradianceModified ? "Irradiance*###Irradiance" : "Irradiance###Irradiance"))
    {
        const ImPlus::ScopeID idScope = {"irradiance"};

        if (ImGui::Button("Generate"))
        {
            generate_cube_map_irradiance();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save") && ctx.irradianceModified)
        {
            ctx.renderer->cubemaps.irradiance.save_as_compressed (
                sq::build_string("assets/", ctx.stage->get_skybox_path(), "/Irradiance.lz4"),
                vk::Format::eE5B9G9R9UfloatPack32, Vec3U(IRRADIANCE_SIZE, IRRADIANCE_SIZE, 6u), 1u
            );
            ctx.irradianceModified = false;
        }

        for (uint face = 0u; face < 6u; ++face)
        {
            ImGui::Image(&ctx.irradiance.descriptorSets[faceMap[face]], {size, size}, {0,1}, {1,0});
            ImPlus::HoverTooltip(faceNames[faceMap[face]]);
            if (face < 5u) ImGui::SameLine();
        }
    }

    //--------------------------------------------------------//

    if (ImGui::CollapsingHeader(ctx.radianceModified ? "Radiance*###Radiance" : "Radiance###Radiance"))
    {
        const ImPlus::ScopeID idScope = {"radiance"};

        if (ImGui::Button("Generate"))
        {
            generate_cube_map_radiance();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save") && ctx.radianceModified)
        {
            ctx.renderer->cubemaps.radiance.save_as_compressed (
                sq::build_string("assets/", ctx.stage->get_skybox_path(), "/Radiance.lz4"),
                vk::Format::eE5B9G9R9UfloatPack32, Vec3U(RADIANCE_SIZE, RADIANCE_SIZE, 6u), RADIANCE_LEVELS
            );
            ctx.radianceModified = false;
        }

        for (uint level = 0u; level < RADIANCE_LEVELS; ++level)
        {
            for (uint face = 0u; face < 6u; ++face)
            {
                ImGui::Image(&ctx.radiance[level].descriptorSets[faceMap[face]], {size, size}, {0,1}, {1,0});
                ImPlus::HoverTooltip("{} ({:.0f}%)"_format(faceNames[faceMap[face]], float(level * 100u) / float(RADIANCE_LEVELS - 1u)));
                if (face < 5u) ImGui::SameLine();
            }
        }
    }
}

//============================================================================//

struct EditorScene::ShrunkCubeMap
{
    sq::ImageStuff image;
    vk::DescriptorSet descriptorSet;
};

EditorScene::ShrunkCubeMap EditorScene::shrink_cube_map_skybox(vk::ImageLayout layout, uint outputSize) const
{
    ShrunkCubeMap result;

    struct {
        vk::RenderPass renderPass;
        std::array<vk::ImageView, 6u> imageViews;
        std::array<vk::Framebuffer, 6u> framebuffers;
        std::array<vk::Pipeline, 6u> pipelines;
    } stuff;

    StageContext& ctx = *mActiveStageContext;
    const auto& vctx = sq::VulkanContext::get();

    // create result image and descriptor set
    {
        result.image.initialise_cube (
            vctx, vk::Format::eR32G32B32A32Sfloat, outputSize, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, false,
            vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor
        );

        const auto imageInfo = vk::DescriptorImageInfo {
            ctx.renderer->samplers.nearestClamp, result.image.view, vk::ImageLayout::eShaderReadOnlyOptimal
        };

        result.descriptorSet = vctx.allocate_descriptor_set(vctx.descriptorPool, mImageProcessSetLayout);
        sq::vk_update_descriptor_set(vctx, result.descriptorSet, sq::DescriptorImageSampler(0u, 0u, imageInfo));
    }

    // create render targets
    {
        const auto attachment = vk::AttachmentDescription {
            {}, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, layout
        };

        const auto reference = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, reference, nullptr, nullptr, nullptr
        };

        stuff.renderPass = vctx.create_render_pass(attachment, subpass, nullptr);
        vctx.set_debug_object_name(stuff.renderPass, "Editor.RenderPass.Shrink");

        for (uint face = 0u; face < 6u; ++face)
        {
            stuff.imageViews[face] = vctx.create_image_view (
                result.image.image, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat,
                vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor, 0u, 1u, face, 1u
            );

            stuff.framebuffers[face] = vctx.create_framebuffer (
                stuff.renderPass, stuff.imageViews[face], Vec2U(outputSize), 1u
            );
        }
    }

    // create pipelines
    for (uint face = 0u; face < 6u; ++face)
    {
        const auto specialise = sq::SpecialisationInfo ( int(SKYBOX_SIZE), int(outputSize) );

        const auto shaderModules = sq::ShaderModules (
            vctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/editor/Shrink.frag.spv", &specialise.info
        );

        stuff.pipelines[face] = sq::vk_create_graphics_pipeline (
            vctx, mImageProcessPipelineLayout, stuff.renderPass, 0u, shaderModules.stages, {},
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eCounterClockwise, false, 0.f, false, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
            },
            vk::PipelineDepthStencilStateCreateInfo {
                {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, 0.f, float(outputSize), float(outputSize) },
            vk::Rect2D { {0, 0}, {outputSize, outputSize} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );
        vctx.set_debug_object_name(stuff.pipelines[face], "Editor.Pipeline.Shrink");
    }

    //--------------------------------------------------------//

    // time to get to work!
    {
        auto cmdbuf = sq::OneTimeCommands(vctx);

        for (uint face = 0u; face < 6u; ++face)
        {
            cmdbuf->bindPipeline(vk::PipelineBindPoint::eGraphics, stuff.pipelines[face]);
            cmdbuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mImageProcessPipelineLayout, 0u, ctx.skybox.descriptorSets[face], {});

            cmdbuf->beginRenderPass (
                vk::RenderPassBeginInfo {
                    stuff.renderPass, stuff.framebuffers[face], vk::Rect2D({0, 0}, {outputSize, outputSize})
                }, vk::SubpassContents::eInline
            );
            cmdbuf->draw(3u, 1u, 0u, 0u);
            cmdbuf->endRenderPass();
        }
    }

    //--------------------------------------------------------//

    vctx.device.destroy(stuff.renderPass);

    for (auto& imageView : stuff.imageViews)
        vctx.device.destroy(imageView);

    for (auto& framebuffer : stuff.framebuffers)
        vctx.device.destroy(framebuffer);

    for (auto& pipeline : stuff.pipelines)
        vctx.device.destroy(pipeline);

    return result;
}

//============================================================================//

void EditorScene::generate_cube_map_irradiance()
{
    struct {
        sq::ImageStuff image;
        vk::RenderPass renderPass;
        std::array<vk::ImageView, 6u> imageViews;
        std::array<vk::Framebuffer, 6u> framebuffers;
        std::array<vk::Pipeline, 6u> pipelines;
    } stuff;

    StageContext& ctx = *mActiveStageContext;
    const auto& vctx = sq::VulkanContext::get();

    ShrunkCubeMap shrunk = shrink_cube_map_skybox(vk::ImageLayout::eShaderReadOnlyOptimal, RADIANCE_SIZE);

    // create render targets
    {
        stuff.image.initialise_cube (
            vctx, vk::Format::eR32G32B32A32Sfloat, IRRADIANCE_SIZE, 1u, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc, false,
            vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor
        );

        const auto attachment = vk::AttachmentDescription {
            {}, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal
        };

        const auto reference = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, reference, nullptr, nullptr, nullptr
        };

        stuff.renderPass = vctx.create_render_pass(attachment, subpass, nullptr);
        vctx.set_debug_object_name(stuff.renderPass, "Editor.RenderPass.Irradiance");

        for (uint face = 0u; face < 6u; ++face)
        {
            stuff.imageViews[face] = vctx.create_image_view (
                stuff.image.image, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat,
                vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor, 0u, 1u, face, 1u
            );

            stuff.framebuffers[face] = vctx.create_framebuffer (
                stuff.renderPass, stuff.imageViews[face], Vec2U(IRRADIANCE_SIZE), 1u
            );
        }
    }

    // create pipelines
    for (uint face = 0u; face < 6u; ++face)
    {
        const auto specialise = sq::SpecialisationInfo(int(RADIANCE_SIZE), int(IRRADIANCE_SIZE), int(face));

        const auto shaderModules = sq::ShaderModules (
            vctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/editor/Irradiance.frag.spv", &specialise.info
        );

        stuff.pipelines[face] = sq::vk_create_graphics_pipeline (
            vctx, mImageProcessPipelineLayout, stuff.renderPass, 0u, shaderModules.stages, {},
            vk::PipelineInputAssemblyStateCreateInfo {
                {}, vk::PrimitiveTopology::eTriangleList, false
            },
            vk::PipelineRasterizationStateCreateInfo {
                {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                vk::FrontFace::eCounterClockwise, false, 0.f, false, 0.f, 1.f
            },
            vk::PipelineMultisampleStateCreateInfo {
                {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
            },
            vk::PipelineDepthStencilStateCreateInfo {
                {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
            },
            vk::Viewport { 0.f, 0.f, float(IRRADIANCE_SIZE), float(IRRADIANCE_SIZE) },
            vk::Rect2D { {0, 0}, {IRRADIANCE_SIZE, IRRADIANCE_SIZE} },
            vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
            nullptr
        );
        vctx.set_debug_object_name(stuff.pipelines[face], "Editor.Pipeline.Irradiance");
    }

    //--------------------------------------------------------//

    // time to get to work!
    {
        auto cmdbuf = sq::OneTimeCommands(vctx);

        for (uint face = 0u; face < 6u; ++face)
        {
            cmdbuf->bindPipeline(vk::PipelineBindPoint::eGraphics, stuff.pipelines[face]);
            cmdbuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mImageProcessPipelineLayout, 0u, shrunk.descriptorSet, {});

            cmdbuf->beginRenderPass (
                vk::RenderPassBeginInfo {
                    stuff.renderPass, stuff.framebuffers[face], vk::Rect2D({0, 0}, {IRRADIANCE_SIZE, IRRADIANCE_SIZE})
                }, vk::SubpassContents::eInline
            );
            cmdbuf->draw(3u, 1u, 0u, 0u);
            cmdbuf->endRenderPass();
        }
    }

    //--------------------------------------------------------//

    update_cube_map_texture(stuff.image, IRRADIANCE_SIZE, 1u, ctx.renderer->cubemaps.irradiance);

    ctx.irradiance.initialise(*this, 0u, ctx.renderer->cubemaps.irradiance.get_image(), ctx.renderer->samplers.linearClamp);

    ctx.renderer->refresh_options_destroy();
    ctx.renderer->refresh_options_create();

    ctx.irradianceModified = true;

    //--------------------------------------------------------//

    shrunk.image.destroy(vctx);
    vctx.device.free(vctx.descriptorPool, shrunk.descriptorSet);

    stuff.image.destroy(vctx);
    vctx.device.destroy(stuff.renderPass);

    for (auto& imageView : stuff.imageViews)
        vctx.device.destroy(imageView);

    for (auto& framebuffer : stuff.framebuffers)
        vctx.device.destroy(framebuffer);

    for (auto& pipeline : stuff.pipelines)
        vctx.device.destroy(pipeline);
}

//============================================================================//

void EditorScene::generate_cube_map_radiance()
{
    struct {
        sq::ImageStuff image;
        vk::RenderPass renderPass;
        std::array<std::array<vk::ImageView, 6u>, RADIANCE_LEVELS - 1u> imageViews;
        std::array<std::array<vk::Framebuffer, 6u>, RADIANCE_LEVELS - 1u> framebuffers;
        std::array<std::array<vk::Pipeline, 6u>, RADIANCE_LEVELS - 1u>  pipelines;
    } stuff;

    ShrunkCubeMap shrunk = shrink_cube_map_skybox(vk::ImageLayout::eTransferSrcOptimal, RADIANCE_SIZE);

    StageContext& ctx = *mActiveStageContext;
    const auto& vctx = sq::VulkanContext::get();

    // create render targets
    {
        stuff.image.initialise_cube (
            vctx, vk::Format::eR32G32B32A32Sfloat, RADIANCE_SIZE, RADIANCE_LEVELS, vk::SampleCountFlagBits::e1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc, false,
            vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor
        );

        const auto attachment = vk::AttachmentDescription {
            {}, vk::Format::eR32G32B32A32Sfloat, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal
        };

        const auto reference = vk::AttachmentReference { 0u, vk::ImageLayout::eColorAttachmentOptimal };

        const auto subpass = vk::SubpassDescription {
            {}, vk::PipelineBindPoint::eGraphics, nullptr, reference, nullptr, nullptr, nullptr
        };

        stuff.renderPass = vctx.create_render_pass(attachment, subpass, nullptr);
        vctx.set_debug_object_name(stuff.renderPass, "Editor.RenderPass.Radiance");

        for (uint level = 1u; level < RADIANCE_LEVELS; ++level)
        {
            const uint index = level - 1u;
            const uint levelSize = RADIANCE_SIZE / uint(std::exp2(level));

            for (uint face = 0u; face < 6u; ++face)
            {
                stuff.imageViews[index][face] = vctx.create_image_view (
                    stuff.image.image, vk::ImageViewType::e2D, vk::Format::eR32G32B32A32Sfloat,
                    vk::ComponentMapping(), vk::ImageAspectFlagBits::eColor, level, 1u, face, 1u
                );

                stuff.framebuffers[index][face] = vctx.create_framebuffer (
                    stuff.renderPass, stuff.imageViews[index][face], Vec2U(levelSize), 1u
                );
            }
        }
    }

    // create pipelines
    for (uint level = 1u; level < RADIANCE_LEVELS; ++level)
    {
        const uint index = level - 1u;
        const uint levelSize = RADIANCE_SIZE / uint(std::exp2(level));

        for (uint face = 0u; face < 6u; ++face)
        {
            const auto specialise = sq::SpecialisationInfo (
                int(RADIANCE_SIZE), int(levelSize), int(face), float(level) / float(RADIANCE_LEVELS - 1u)
            );

            const auto shaderModules = sq::ShaderModules (
                vctx, "shaders/FullScreenFC.vert.spv", {}, "shaders/editor/Radiance.frag.spv", &specialise.info
            );

            stuff.pipelines[index][face] = sq::vk_create_graphics_pipeline (
                vctx, mImageProcessPipelineLayout, stuff.renderPass, 0u, shaderModules.stages, {},
                vk::PipelineInputAssemblyStateCreateInfo {
                    {}, vk::PrimitiveTopology::eTriangleList, false
                },
                vk::PipelineRasterizationStateCreateInfo {
                    {}, false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone,
                    vk::FrontFace::eCounterClockwise, false, 0.f, false, 0.f, 1.f
                },
                vk::PipelineMultisampleStateCreateInfo {
                    {}, vk::SampleCountFlagBits::e1, false, 0.f, nullptr, false, false
                },
                vk::PipelineDepthStencilStateCreateInfo {
                    {}, false, false, {}, false, false, {}, {}, 0.f, 0.f
                },
                vk::Viewport { 0.f, 0.f, float(levelSize), float(levelSize) },
                vk::Rect2D { {0, 0}, {levelSize, levelSize} },
                vk::PipelineColorBlendAttachmentState { false, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlags(0b1111) },
                nullptr
            );
            vctx.set_debug_object_name(stuff.pipelines[index][face], "Editor.Pipeline.Radiance");
        }
    }

    //--------------------------------------------------------//

    // time to get to work!
    {
        auto cmdbuf = sq::OneTimeCommands(vctx);

        // copy shrunk skybox into level zero
        {
            auto cmdbuf = sq::OneTimeCommands(vctx);

            sq::vk_pipeline_barrier_image_memory (
                cmdbuf.cmdbuf, stuff.image.image, vk::DependencyFlags(),
                vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 6u
            );

            cmdbuf->copyImage (
                shrunk.image.image, vk::ImageLayout::eTransferSrcOptimal, stuff.image.image, vk::ImageLayout::eTransferDstOptimal,
                vk::ImageCopy (
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 6u), vk::Offset3D(0, 0, 0),
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 6u), vk::Offset3D(0, 0, 0),
                    vk::Extent3D(RADIANCE_SIZE, RADIANCE_SIZE, 1u)
                )
            );

            sq::vk_pipeline_barrier_image_memory (
                cmdbuf.cmdbuf, shrunk.image.image, vk::DependencyFlags(),
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderRead,
                vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
                vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 6u
            );

            sq::vk_pipeline_barrier_image_memory (
                cmdbuf.cmdbuf, stuff.image.image, vk::DependencyFlags(),
                vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe,
                vk::AccessFlagBits::eTransferWrite, vk::AccessFlags(),
                vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 6u
            );
        }

        for (uint level = 1u; level < RADIANCE_LEVELS; ++level)
        {
            const uint index = level - 1u;
            const uint levelSize = RADIANCE_SIZE / uint(std::exp2(level));

            for (uint face = 0u; face < 6u; ++face)
            {
                cmdbuf->bindPipeline(vk::PipelineBindPoint::eGraphics, stuff.pipelines[index][face]);
                cmdbuf->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mImageProcessPipelineLayout, 0u, shrunk.descriptorSet, {});

                cmdbuf->beginRenderPass (
                    vk::RenderPassBeginInfo {
                        stuff.renderPass, stuff.framebuffers[index][face], vk::Rect2D({0, 0}, {levelSize, levelSize})
                    }, vk::SubpassContents::eInline
                );
                cmdbuf->draw(3u, 1u, 0u, 0u);
                cmdbuf->endRenderPass();
            }
        }
    }

    //--------------------------------------------------------//

    update_cube_map_texture(stuff.image, RADIANCE_SIZE, RADIANCE_LEVELS, ctx.renderer->cubemaps.radiance);

    for (uint level = 0u; level < RADIANCE_LEVELS; ++level)
        ctx.radiance[level].initialise(*this, level, ctx.renderer->cubemaps.radiance.get_image(), ctx.renderer->samplers.linearClamp);

    ctx.renderer->refresh_options_destroy();
    ctx.renderer->refresh_options_create();

    ctx.radianceModified = true;

    //--------------------------------------------------------//

    shrunk.image.destroy(vctx);
    vctx.device.free(vctx.descriptorPool, shrunk.descriptorSet);

    stuff.image.destroy(vctx);
    vctx.device.destroy(stuff.renderPass);

    for (auto& imageViews : stuff.imageViews)
        for (auto& imageView : imageViews)
            vctx.device.destroy(imageView);

    for (auto& framebuffers : stuff.framebuffers)
        for (auto& framebuffer : framebuffers)
            vctx.device.destroy(framebuffer);

    for (auto& pipelines : stuff.pipelines)
        for (auto& pipeline : pipelines)
            vctx.device.destroy(pipeline);
}

//============================================================================//

void EditorScene::update_cube_map_texture(sq::ImageStuff source, uint size, uint levels, sq::Texture& texture)
{
    const auto& ctx = sq::VulkanContext::get();

    const size_t pixelCount = sq::Texture::compute_buffer_size(Vec3U(size, size, 6u), levels, 1u);

    // copy all mip levels into a host buffer
    auto buffer = sq::StagingBuffer(ctx, false, true, pixelCount * sizeof(Vec4F));
    {
        auto cmdbuf = sq::OneTimeCommands(ctx);

        size_t levelBufferOffset = 0u;
        uint levelSize = size;

        for (uint level = 0u; level < levels; ++level)
        {
            cmdbuf->copyImageToBuffer (
                source.image, vk::ImageLayout::eTransferSrcOptimal, buffer.buffer,
                vk::BufferImageCopy (
                    levelBufferOffset, 0u, 0u,
                    vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, level, 0u, 6u),
                    vk::Offset3D(0, 0, 0), vk::Extent3D(levelSize, levelSize, 1u)
                )
            );

            levelBufferOffset += levelSize * levelSize * 6u * sizeof(Vec4F);
            levelSize = std::max(1u, levelSize / 2u);
        }
    }

    // compress hdr colour values to e5bgr9
    auto compressed = std::unique_ptr<uint32_t[]>(new uint32_t[pixelCount]());
    {
        const auto vec4_to_e5bgr9 = [](Vec4F vec) { return sq::maths::hdr_to_e5bgr9(Vec3F(vec)); };

        const Vec4F* mapped = reinterpret_cast<Vec4F*>(buffer.memory.map());
        std::transform(mapped, mapped + pixelCount, compressed.get(), vec4_to_e5bgr9);
        buffer.memory.unmap();
    }

    // upload the new data to the texture
    {
        sq::Texture::Config config;
        config.format = vk::Format::eE5B9G9R9UfloatPack32;
        config.wrapX = config.wrapY = config.wrapZ = vk::SamplerAddressMode::eClampToEdge;
        config.swizzle = vk::ComponentMapping();
        config.filter = sq::Texture::FilterMode::Linear;
        config.mipmaps = levels > 1u ? sq::Texture::MipmapsMode::Load : sq::Texture::MipmapsMode::Disable;
        config.size = Vec3U(size, size, 6u);
        config.mipLevels = levels;

        texture = sq::Texture();
        texture.initialise_cube(config);
        texture.load_from_memory(compressed.get(), pixelCount * sizeof(uint32_t), config);
    }
}