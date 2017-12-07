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

    void render_blobs(const std::vector<HitBlob*>& blobs);

    void render_blobs(const std::vector<HurtBlob*>& blobs);

private: //===================================================//

    sq::Program mBlobShader;

    sq::Mesh mSphereMesh;
    sq::Mesh mCapsuleMesh;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
