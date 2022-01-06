#pragma once

#include "setup.hpp"

#include "main/Resources.hpp"

#include "render/DrawItem.hpp"

#include <sqee/objects/Texture.hpp>
#include <sqee/vk/SwapBuffer.hpp>
#include <sqee/vk/Wrappers.hpp>

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

    void set_camera(Camera& camera) { mCamera = &camera; };

    Camera& get_camera() { return *mCamera; }

    const Camera& get_camera() const { return *mCamera; }

    //--------------------------------------------------------//

    /// Create some DrawItems from a vector of definitions.
    int64_t create_draw_items(const std::vector<DrawItemDef>& defs,
                              const sq::Swapper<vk::DescriptorSet>& descriptorSet,
                              const std::map<TinyString, const bool*>& conditions);

    /// Delete all DrawItems with the given group id.
    void delete_draw_items(int64_t groupId);

    void update_cubemap_descriptor_sets();

    //--------------------------------------------------------//

    void integrate_camera(float blend);

    void integrate_particles(float blend, const ParticleSystem& system);

    void integrate_debug(float blend, const World& world);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

    void populate_final_pass(vk::CommandBuffer cmdbuf);

    //--------------------------------------------------------//

    struct {
        float exposure;
        float contrast;
        float black;
    } tonemap;

    struct {
        sq::SwapBuffer camera;
        sq::SwapBuffer environment;
    } ubos;

    struct {
        sq::Texture skybox;
        sq::Texture irradiance;
        sq::Texture radiance;
    } cubemaps;

    struct {
        vk::DescriptorSetLayout gbuffer;
        vk::DescriptorSetLayout shadow;
        vk::DescriptorSetLayout shadowMiddle;
        vk::DescriptorSetLayout depthMipGen;
        vk::DescriptorSetLayout ssao;
        vk::DescriptorSetLayout ssaoBlur;
        vk::DescriptorSetLayout skybox;
        vk::DescriptorSetLayout lighting;
        vk::DescriptorSetLayout transparent;
        vk::DescriptorSetLayout particles;
        vk::DescriptorSetLayout composite;
        vk::DescriptorSetLayout object;
    } setLayouts;

    struct {
        vk::PipelineLayout gbuffer;
        vk::PipelineLayout shadow;
        vk::PipelineLayout shadowMiddle;
        vk::PipelineLayout depthMipGen;
        vk::PipelineLayout ssao;
        vk::PipelineLayout ssaoBlur;
        vk::PipelineLayout skybox;
        vk::PipelineLayout lighting;
        vk::PipelineLayout transparent;
        vk::PipelineLayout particles;
        vk::PipelineLayout composite;
    } pipelineLayouts;

    struct {
        sq::Swapper<vk::DescriptorSet> gbuffer;
        sq::Swapper<vk::DescriptorSet> shadow;
        vk::DescriptorSet              shadowMiddle;
        sq::Swapper<vk::DescriptorSet> ssao;
        sq::Swapper<vk::DescriptorSet> ssaoBlur;
        sq::Swapper<vk::DescriptorSet> skybox;
        sq::Swapper<vk::DescriptorSet> lighting;
        sq::Swapper<vk::DescriptorSet> transparent;
        sq::Swapper<vk::DescriptorSet> particles;
        vk::DescriptorSet              composite;
    } sets;

    struct {
        vk::Pipeline shadowMiddle;
        vk::Pipeline ssao;
        vk::Pipeline ssaoBlur;
        vk::Pipeline skybox;
        vk::Pipeline lighting;
        vk::Pipeline particles;
        vk::Pipeline composite;
    } pipelines;

    struct {
        sq::ImageStuff depthStencil;
        vk::ImageView depthView;
        sq::ImageStuff albedoRoughness;
        sq::ImageStuff normalMetallic;
        sq::ImageStuff shadowFront;
        sq::ImageStuff shadowBack;
        sq::ImageStuff shadowMiddle;
        sq::ImageStuff depthMips;
        sq::ImageStuff ssao;
        sq::ImageStuff ssaoBlur;
        sq::ImageStuff colour;
    } images;

    struct {
        vk::Sampler nearestClamp;
        vk::Sampler linearClamp;
        vk::Sampler depthCompare;
        vk::Sampler depthMips;
    } samplers;

    struct {
        sq::RenderPassStuff gbuffer;
        sq::RenderPassStuff shadow;
        sq::RenderPassStuff ssao;
        sq::RenderPassStuff ssaoBlur;
        sq::RenderPassStuff hdr;
    } passes;

    //--------------------------------------------------------//

    enum class TimeStamp : uint
    {
        BeginGbuffer,
        Opaque,
        EndGbuffer,
        Shadows,
        ShadowAverage,
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
    static constexpr uint NUM_TIME_STAMPS = 19u;

    const std::array<double, NUM_TIME_STAMPS>& get_frame_timings() { return mFrameTimings; }

    void write_time_stamp(vk::CommandBuffer cmdbuf [[maybe_unused]], TimeStamp stamp [[maybe_unused]])
    {
        cmdbuf.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, mTimestampQueryPool.front, uint(stamp) + 1u);
    }

private: //===================================================//

    void impl_initialise_layouts();

    void impl_create_render_targets();
    void impl_destroy_render_targets();

    void impl_create_shadow_stuff();
    void impl_destroy_shadow_stuff();

    void impl_create_depth_mipmap_stuff();
    void impl_destroy_depth_mipmap_stuff();

    void impl_create_ssao_stuff();
    void impl_destroy_ssao_stuff();

    void impl_create_pipelines();
    void impl_destroy_pipelines();

    //--------------------------------------------------------//

    struct DepthMipGenStuff
    {
        vk::ImageView srcView; // reference
        vk::ImageView destView;
        sq::RenderPassStuff pass;
        vk::Pipeline pipeline;
        vk::DescriptorSet descriptorSet;
        Vec2U dimensions;
    };

    std::vector<DepthMipGenStuff> mDepthMipGenStuff;

    Camera* mCamera = nullptr;

    std::unique_ptr<DebugRenderer> mDebugRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;

    sq::Texture mLutTexture;

    sq::PassConfig* mPassConfigGbuffer = nullptr;
    sq::PassConfig* mPassConfigShadowFront = nullptr;
    sq::PassConfig* mPassConfigShadowBack = nullptr;
    sq::PassConfig* mPassConfigTransparent = nullptr;

    std::vector<DrawItem> mDrawItemsGbuffer;
    std::vector<DrawItem> mDrawItemsShadowFront;
    std::vector<DrawItem> mDrawItemsShadowBack;
    std::vector<DrawItem> mDrawItemsTransparent;

    bool mNeedDestroyShadow = false;
    bool mNeedDestroySSAO = false;

    int64_t mCurrentGroupId = -1;

    //--------------------------------------------------------//

    std::array<double, NUM_TIME_STAMPS> mFrameTimings {};
    sq::Swapper<vk::QueryPool> mTimestampQueryPool;
};

//============================================================================//

} // namespace sts
