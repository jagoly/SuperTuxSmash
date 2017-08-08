#pragma once

#include <sqee/gl/Textures.hpp>
#include <sqee/gl/UniformBuffer.hpp>
#include <sqee/gl/Program.hpp>

#include <sqee/render/Mesh.hpp>

#include "render/RenderObject.hpp"

#include "fighters/Sara_Fighter.hpp"

//============================================================================//

namespace sts {

class Sara_Render final : public RenderObject
{
public: //====================================================//

    Sara_Render(const Renderer& renderer, const Sara_Fighter& fighter);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

private: //===================================================//

    sq::Mesh MESH_Sara;

    sq::Texture2D TX_Main_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Main_spec { sq::Texture::Format::R8_UN };

    sq::Texture2D TX_Hair_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Hair_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Hair_mask { sq::Texture::Format::R8_UN };

    sq::Program PROG_Main;
    sq::Program PROG_Hair;

    //--------------------------------------------------------//

    sq::UniformBuffer mUbo;

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;

    //--------------------------------------------------------//

    const Sara_Fighter& fighter;
};

} // namespace sts
