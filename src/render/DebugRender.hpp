#pragma once

#include "setup.hpp"

#include <sqee/gl/FixedBuffer.hpp>
#include <sqee/gl/Program.hpp>
#include <sqee/gl/VertexArray.hpp>
#include <sqee/objects/Mesh.hpp>

namespace sts {

//============================================================================//

class DebugRenderer final : sq::NonCopyable
{
public: //====================================================//

    DebugRenderer(Renderer& renderer);

    void refresh_options();

    //--------------------------------------------------------//

    void render_hit_blobs(const std::vector<HitBlob*>& blobs);

    void render_hurt_blobs(const std::vector<HurtBlob*>& blobs);

    void render_diamond(const Fighter& fighter);

    void render_skeleton(const Fighter& fighter);

private: //===================================================//

    sq::Program mBlobShader;
    sq::Program mLinesShader;

    sq::Mesh mSphereMesh;
    sq::Mesh mCapsuleMesh;
    sq::Mesh mDiamondMesh;

    sq::VertexArray mLineVertexArray;
    sq::FixedBuffer mLineVertexBuffer;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
