#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include "render/DrawItem.hpp"

#include <sqee/objects/Texture.hpp>
#include <sqee/vk/SwapBuffer.hpp>

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

    void integrate_camera(float blend);

    void integrate_particles(float blend, const ParticleSystem& system);

    void integrate_debug(float blend, const FightWorld& world);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

    void populate_final_pass(vk::CommandBuffer cmdbuf);

    //--------------------------------------------------------//

    struct {
        vk::DescriptorSetLayout camera;
        vk::DescriptorSetLayout gbuffer;
        vk::DescriptorSetLayout depthMipGen;
        vk::DescriptorSetLayout ssao;
        vk::DescriptorSetLayout ssaoBlur;
        vk::DescriptorSetLayout skybox;
        vk::DescriptorSetLayout lightDefault;
        vk::DescriptorSetLayout object;
        vk::DescriptorSetLayout composite;
    } setLayouts;

    struct {
        vk::PipelineLayout standard;
        vk::PipelineLayout depthMipGen;
        vk::PipelineLayout ssao;
        vk::PipelineLayout ssaoBlur;
        vk::PipelineLayout skybox;
        vk::PipelineLayout lightDefault;
        vk::PipelineLayout composite;
    } pipelineLayouts;

    struct {
        sq::Swapper<vk::DescriptorSet> camera;
        vk::DescriptorSet ssao;
        vk::DescriptorSet ssaoBlur;
        vk::DescriptorSet skybox;
        sq::Swapper<vk::DescriptorSet> lightDefault;
        vk::DescriptorSet composite;
    } sets;

    struct {
        vk::Pipeline ssao;
        vk::Pipeline ssaoBlur;
        vk::Pipeline skybox;
        vk::Pipeline lightDefault;
        vk::Pipeline composite;
    } pipelines;

    struct {
        vk::Image depthStencil;
        sq::VulkanMemory depthStencilMem;
        vk::ImageView depthStencilView;
        vk::ImageView depthView;
        vk::ImageView stencilView;
        vk::Image albedoRoughness;
        sq::VulkanMemory albedoRoughnessMem;
        vk::ImageView albedoRoughnessView;
        vk::Image normalMetallic;
        sq::VulkanMemory normalMetallicMem;
        vk::ImageView normalMetallicView;
        vk::Image depthMips;
        sq::VulkanMemory depthMipsMem;
        vk::ImageView depthMipsView;
        vk::Image ssao;
        sq::VulkanMemory ssaoMem;
        vk::ImageView ssaoView;
        vk::Image ssaoBlur;
        sq::VulkanMemory ssaoBlurMem;
        vk::ImageView ssaoBlurView;
        vk::Image colour;
        sq::VulkanMemory colourMem;
        vk::ImageView colourView;
    } images;

    struct {
        vk::Sampler nearestClamp;
        vk::Sampler linearClamp;
        vk::Sampler depthMips;
    } samplers;

    struct {
        vk::RenderPass gbufferRenderPass;
        vk::Framebuffer gbufferFramebuffer;
        vk::RenderPass ssaoRenderPass;
        vk::Framebuffer ssaoFramebuffer;
        vk::RenderPass ssaoBlurRenderPass;
        vk::Framebuffer ssaoBlurFramebuffer;
        vk::RenderPass hdrRenderPass;
        vk::Framebuffer hdrFramebuffer;
    } targets;

    //--------------------------------------------------------//

    enum class TimeStamp : uint
    {
        BeginGbuffer,
        Opaque,
        EndGbuffer,
        DepthMipGen,
        SSAO,
        BlurSSAO,
        BeginHDR,
        Skybox,
        LightDefault,
        Transparent,
        Particles,
        EndHDR,
        BeginFinal,
        Composite,
        Debug,
        Gui,
        EndFinal
    };
    static constexpr uint NUM_TIME_STAMPS = 17u;

    const std::array<double, NUM_TIME_STAMPS>& get_frame_timings() { return mFrameTimings; }

    void write_time_stamp(vk::CommandBuffer cmdbuf [[maybe_unused]], TimeStamp stamp [[maybe_unused]])
    {
        cmdbuf.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, mTimestampQueryPool.front, uint(stamp) + 1u);
    }

private: //===================================================//

    void impl_initialise_layouts();

    void impl_create_render_targets();

    void impl_destroy_render_targets();

    void impl_create_pipelines();

    void impl_destroy_pipelines();

    void impl_create_depth_mip_gen_stuff();

    void impl_destroy_depth_mip_gen_stuff();

    bool mNeedDestroySSAO = false;

    //--------------------------------------------------------//

    sq::Texture mLutTexture;

    // todo: should be part of the stage
    sq::Texture mSkyboxTexture;
    sq::Texture mIrradianceTexture;
    sq::Texture mRadianceTexture;

    //--------------------------------------------------------//

    struct DepthMipGenStuff
    {
        vk::ImageView srcView; // reference
        vk::ImageView destView;
        vk::RenderPass renderPass;
        vk::Framebuffer framebuffer;
        vk::Pipeline pipeline;
        vk::DescriptorSet descriptorSet;
        Vec2U dimensions;
    };

    std::vector<DepthMipGenStuff> mDepthMipGenStuff;

    std::unique_ptr<Camera> mCamera;

    std::unique_ptr<DebugRenderer> mDebugRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;

    sq::SwapBuffer mCameraUbo;
    sq::SwapBuffer mLightUbo;

    sq::PassConfig* mPassConfigOpaque = nullptr;
    sq::PassConfig* mPassConfigTransparent = nullptr;

    std::vector<DrawItem> mDrawItemsOpaque;
    std::vector<DrawItem> mDrawItemsTransparent;

    int64_t mCurrentGroupId = -1;

    //--------------------------------------------------------//

    std::array<double, NUM_TIME_STAMPS> mFrameTimings {};
    sq::Swapper<vk::QueryPool> mTimestampQueryPool;
};

//============================================================================//

} // namespace sts
