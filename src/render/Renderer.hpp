#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include "render/DrawItem.hpp"

#include <sqee/vk/SwapBuffer.hpp>
#include <sqee/vk/VulkTexture.hpp>

//============================================================================//

namespace sq { class Window; }

namespace sts {

class DebugRenderer;
class ParticleRenderer;

//============================================================================//

class Renderer final : sq::NonCopyable
{
public: //====================================================//

    Renderer(const sq::Window& window, const Options& options, ResourceCaches& caches);

    ~Renderer();

    //--------------------------------------------------------//

    const sq::Window& window;
    const Options& options;
    ResourceCaches& caches;

    //--------------------------------------------------------//

    void refresh_options_destroy();

    void refresh_options_create();

    //--------------------------------------------------------//

    void set_camera(std::unique_ptr<Camera> camera);

    Camera& get_camera() { return *mCamera; }

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    /// Create some DrawItems from a vector of definitions.
    int64_t create_draw_items(const std::vector<DrawItemDef>& defs,
                              const sq::Swapper<vk::DescriptorSet>& descriptorSet,
                              std::map<TinyString, const bool*> conditions);

    /// Delete all DrawItems with the given group id.
    void delete_draw_items(int64_t groupId);

    //--------------------------------------------------------//

    void integrate(float blend);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

    void populate_final_pass(vk::CommandBuffer cmdbuf);

    void render_particles(const ParticleSystem& system, float blend);

    //--------------------------------------------------------//

    DebugRenderer& get_debug_renderer() { return *mDebugRenderer; }

    //--------------------------------------------------------//

    struct {
        vk::DescriptorSetLayout camera;
        vk::DescriptorSetLayout skybox;
        vk::DescriptorSetLayout light;
        vk::DescriptorSetLayout object;
        vk::DescriptorSetLayout composite;
    } setLayouts;

    struct {
        vk::PipelineLayout skybox;
        vk::PipelineLayout standard;
        vk::PipelineLayout composite;
    } pipelineLayouts;

    struct {
        sq::Swapper<vk::DescriptorSet> camera;
        sq::Swapper<vk::DescriptorSet> skybox;
        sq::Swapper<vk::DescriptorSet> light;
        vk::DescriptorSet composite;
    } sets;

    struct {
        vk::Pipeline skybox;
        vk::Pipeline composite;
    } pipelines;

    struct {
        vk::Image msColour;
        sq::VulkanMemory msColourMem;
        vk::ImageView msColourView;
        vk::Image msDepth;
        sq::VulkanMemory msDepthMem;
        vk::ImageView msDepthView;
        vk::Image resolveColour;
        sq::VulkanMemory resolveColourMem;
        vk::ImageView resolveColourView;
        vk::Image resolveDepth;
        sq::VulkanMemory resolveDepthMem;
        vk::ImageView resolveDepthView;
    } images;

    struct {
        vk::Sampler resolveColour;
        vk::Sampler resolveDepth;
    } samplers;

    struct {
        vk::RenderPass msRenderPass;
        vk::Framebuffer msFramebuffer;
//        vk::RenderPass fxRenderPass;
//        vk::Framebuffer fxFramebuffer;
    } targets;

private: //===================================================//

    void impl_initialise_layouts();

    void impl_create_render_targets();

    void impl_destroy_render_targets();

    void impl_create_pipelines();

    void impl_destroy_pipelines();

    //--------------------------------------------------------//

    // todo: should be part of the stage
    sq::VulkTexture mSkyboxTexture;

    //--------------------------------------------------------//

    std::unique_ptr<Camera> mCamera;

    std::unique_ptr<DebugRenderer> mDebugRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;

    sq::SwapBuffer mCameraUbo;
    sq::SwapBuffer mLightUbo;

    std::vector<DrawItem> mDrawItems;

    int64_t mCurrentGroupId = -1;
};

//============================================================================//

} // namespace sts
