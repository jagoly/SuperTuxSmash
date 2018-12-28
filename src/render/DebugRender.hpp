#pragma once

#include "render/Renderer.hpp"

//============================================================================//

namespace sts {

class DebugRender final : sq::NonCopyable
{
public: //====================================================//

    DebugRender(Renderer& renderer);

    void refresh_options();

    //--------------------------------------------------------//

    void render_blobs(const Vector<HitBlob*>& blobs);

    void render_blobs(const Vector<HurtBlob*>& blobs);

private: //===================================================//

    sq::Program mBlobShader;
    sq::Program mArrowShader;

    sq::Mesh mSphereMesh;
    sq::Mesh mCapsuleMesh;

    sq::VertexArray mLineVertexArray;
    sq::FixedBuffer mLineVertexBuffer;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
