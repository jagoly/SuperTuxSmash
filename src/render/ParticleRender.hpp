#pragma once

#include "setup.hpp"

#include "game/ParticleSystem.hpp"

#include <sqee/vk/SwapBuffer.hpp>
#include <sqee/vk/VulkTexture.hpp>
#include <sqee/vk/Pipeline.hpp>

namespace sts {

//============================================================================//

class ParticleRenderer final : sq::NonCopyable
{
public: //====================================================//

    ParticleRenderer(Renderer& renderer);

    ~ParticleRenderer();

    void refresh_options_destroy();

    void refresh_options_create();

    //--------------------------------------------------------//

    void swap_sets();

    void integrate_set(float blend, const ParticleSystem& system);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

private: //===================================================//

    Renderer& renderer;

    //--------------------------------------------------------//

    vk::DescriptorSetLayout mDescriptorSetLayout;
    vk::PipelineLayout mPipelineLayout;

    vk::DescriptorSet mDescriptorSet;
    vk::Pipeline mPipeline;

    sq::SwapBuffer mVertexBuffer;

    sq::VulkTexture mTexture;

    //--------------------------------------------------------//

    struct ParticleSetInfo
    {
        //TexArrayHandle texture;
        uint16_t startIndex;
        uint16_t vertexCount;
        float averageDepth;
    };

    std::vector<ParticleSetInfo> mParticleSetInfo;
    std::vector<ParticleSetInfo> mParticleSetInfoKeep;

    std::vector<ParticleVertex> mVertices;
};

} // namespace sts
