#pragma once

#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>
#include <sqee/gl/Program.hpp>

#include <sqee/render/Armature.hpp>
#include <sqee/render/Mesh.hpp>

#include "render/RenderEntity.hpp"

#include "fighters/Tux_Fighter.hpp"

//============================================================================//

namespace sts {

class Tux_Render final : public RenderObject
{
public: //====================================================//

    Tux_Render(const Renderer& renderer, const Tux_Fighter& fighter);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

private: //===================================================//

    sq::Mesh MESH_Tux;

    sq::Texture2D TX_Tux_diff { sq::Texture::Format::RGB8_UN };

    sq::Program PROG_Tux;

    //--------------------------------------------------------//

    sq::UniformBuffer mUbo;

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;

    //--------------------------------------------------------//

    const Tux_Fighter& fighter;
};

} // namespace sts
