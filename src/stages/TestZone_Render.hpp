#pragma once

#include "render/RenderObject.hpp"

#include "render/ResourceCaches.hpp"

namespace sts {

class TestZone_Stage;

//============================================================================//

class TestZone_Render final : public RenderObject
{
public: //====================================================//

    TestZone_Render(Renderer& renderer, const TestZone_Stage& stage);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

    void render_alpha() override {}

private: //===================================================//

    MeshHandle MESH_Mesh;

    TextureHandle TEX_Diff;

    ProgramHandle PROG_Main;

    //--------------------------------------------------------//

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;

    //--------------------------------------------------------//

    const TestZone_Stage& stage;
};

//============================================================================//

} // namespace sts
