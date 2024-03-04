#pragma once

#include "setup.hpp"

#include <sqee/objects/Mesh.hpp>
#include <sqee/vk/SwapBuffer.hpp>

namespace sts {

//============================================================================//

class DebugRenderer final
{
public: //====================================================//

    DebugRenderer(Renderer& renderer);

    SQEE_COPY_DELETE(DebugRenderer)
    SQEE_MOVE_DELETE(DebugRenderer)

    ~DebugRenderer();

    void refresh_options_destroy();

    void refresh_options_create();

    void integrate(float blend, const World& world);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

private: //===================================================//

    Renderer& renderer;

    //--------------------------------------------------------//

    void impl_integrate_hit_blobs(const std::vector<HitBlob>& blobs);

    void impl_integrate_hurt_blobs(const std::vector<HurtBlob>& blobs);

    void impl_integrate_diamond(const Fighter& fighter);

    void impl_integrate_skeleton(const Entity& entity);

    //--------------------------------------------------------//

    struct DrawBlob
    {
        Mat4F matrix;
        Vec4F colour;
        const sq::Mesh* mesh;
        int subMesh;
        int sortValue;
    };

    struct Triangle
    {
        Vec4F pointA, colourA;
        Vec4F pointB, colourB;
        Vec4F pointC, colourC;
    };

    struct Line
    {
        Vec4F pointA, colourA;
        Vec4F pointB, colourB;
    };

    //--------------------------------------------------------//

    vk::PipelineLayout mBlobPipelineLayout;
    vk::PipelineLayout mPrimitivesPipelineLayout;

    vk::Pipeline mBlobPipeline;
    vk::Pipeline mTrianglesPipeline;
    vk::Pipeline mLinesPipeline;

    sq::SwapBuffer mTrianglesVertexBuffer;
    sq::SwapBuffer mThickLinesVertexBuffer;
    sq::SwapBuffer mThinLinesVertexBuffer;

    sq::Mesh mSphereMesh;
    sq::Mesh mCapsuleMesh;

    std::vector<DrawBlob> mDrawBlobs;

    uint mTriangleCount = 0u;
    uint mThickLineCount = 0u;
    uint mThinLineCount = 0u;
};

//============================================================================//

} // namespace sts
