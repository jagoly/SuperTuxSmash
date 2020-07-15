#pragma once

#include "render/Renderer.hpp"

//============================================================================//

namespace sts {

class DebugRenderer final : sq::NonCopyable
{
public: //====================================================//

    DebugRenderer(Renderer& renderer);

    void refresh_options();

    //--------------------------------------------------------//

    void render_hit_blobs(const Vector<HitBlob*>& blobs);

    void render_hurt_blobs(const Vector<HurtBlob*>& blobs);

    void render_diamond(Vec2F position, const struct LocalDiamond& diamond);

    void render_skeleton(const Fighter& fighter);

private: //===================================================//

    sq::Program mBlobShader;
    sq::Program mArrowShader;

    sq::Mesh mSphereMesh;
    sq::Mesh mCapsuleMesh;
    sq::Mesh mDiamondMesh;

    sq::VertexArray mLineVertexArray;
    sq::FixedBuffer mLineVertexBuffer;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
