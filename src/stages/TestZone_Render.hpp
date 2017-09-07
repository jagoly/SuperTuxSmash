#pragma once

#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>
#include <sqee/gl/Program.hpp>

#include <sqee/render/Mesh.hpp>

#include "render/RenderObject.hpp"

#include "stages/TestZone_Stage.hpp"

//============================================================================//

namespace sts {

class TestZone_Render final : public RenderObject
{
public: //====================================================//

    TestZone_Render(const Renderer& renderer, const TestZone_Stage& stage);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

private: //===================================================//

    sq::Mesh MESH_Mesh;

    sq::Texture2D TEX_Diff { sq::Texture::Format::RGB8_UN };

    sq::Program PROG_Main;

    //--------------------------------------------------------//

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;

    //--------------------------------------------------------//

    const TestZone_Stage& stage;
};

} // namespace sts
