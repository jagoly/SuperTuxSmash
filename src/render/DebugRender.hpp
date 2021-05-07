#pragma once

#include "setup.hpp"

#include <sqee/vk/VulkMesh.hpp>
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

    //--------------------------------------------------------//

    void render_hit_blobs(const std::vector<HitBlob*>& blobs);

    void render_hurt_blobs(const std::vector<HurtBlob*>& blobs);

    void render_diamond(const Fighter& fighter);

    void render_skeleton(const Fighter& fighter);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

private: //===================================================//

    Renderer& renderer;

    //--------------------------------------------------------//

    vk::PipelineLayout mBlobPipelineLayout;
    vk::PipelineLayout mLinesPipelineLayout;

    vk::Pipeline mBlobPipeline;
    vk::Pipeline mLinesPipeline;

    sq::SwapBuffer mThickLinesVertexBuffer;
    sq::SwapBuffer mThinLinesVertexBuffer;

    sq::VulkMesh mSphereMesh;
    sq::VulkMesh mCapsuleMesh;
    sq::VulkMesh mDiamondMesh;

    //--------------------------------------------------------//

    struct DrawBlob
    {
        Mat4F matrix;
        Vec4F colour;
        const sq::VulkMesh* mesh;
        int subMesh;
        int sortValue;
        bool operator<(const DrawBlob& other) { return sortValue < other.sortValue; }
    };

    struct Line
    {
        Vec4F pointA;
        Vec4F colourA;
        Vec4F pointB;
        Vec4F colourB;
    };

    std::vector<DrawBlob> mDrawBlobs;

    uint mThickLineCount = 0u;
    uint mThinLineCount = 0u;
};

//============================================================================//

} // namespace sts
