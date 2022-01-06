#pragma once

#include "setup.hpp"

#include <sqee/objects/Mesh.hpp>
#include <sqee/vk/SwapBuffer.hpp>

namespace sts {

//============================================================================//

class DebugRenderer final : sq::NonCopyable
{
public: //====================================================//

    DebugRenderer(Renderer& renderer);

    ~DebugRenderer();

    void refresh_options_destroy();

    void refresh_options_create();

    void integrate(float blend, const World& world);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

private: //===================================================//

    Renderer& renderer;

    //--------------------------------------------------------//

    void impl_integrate_hit_blobs(const std::vector<HitBlob*>& blobs);

    void impl_integrate_hurt_blobs(const std::vector<HurtBlob*>& blobs);

    void impl_integrate_diamond(const Fighter& fighter);

    void impl_integrate_skeleton(const Fighter& fighter);

    //--------------------------------------------------------//

    struct DrawBlob
    {
        Mat4F matrix;
        Vec4F colour;
        const sq::Mesh* mesh;
        int subMesh;
        int sortValue;
    };

    struct Line
    {
        Vec4F pointA;
        Vec4F colourA;
        Vec4F pointB;
        Vec4F colourB;
    };

    //--------------------------------------------------------//

    vk::PipelineLayout mBlobPipelineLayout;
    vk::PipelineLayout mLinesPipelineLayout;

    vk::Pipeline mBlobPipeline;
    vk::Pipeline mLinesPipeline;

    sq::SwapBuffer mThickLinesVertexBuffer;
    sq::SwapBuffer mThinLinesVertexBuffer;

    sq::Mesh mSphereMesh;
    sq::Mesh mCapsuleMesh;
    sq::Mesh mDiamondMesh;

    std::vector<DrawBlob> mDrawBlobs;

    uint mThickLineCount = 0u;
    uint mThinLineCount = 0u;
};

//============================================================================//

} // namespace sts
