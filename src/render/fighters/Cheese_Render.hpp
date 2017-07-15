#pragma once

#include <sqee/gl/Textures.hpp>
#include <sqee/gl/Program.hpp>

#include <sqee/render/Mesh.hpp>

#include "render/RenderEntity.hpp"

//============================================================================//

namespace sts {

class Cheese_Render final : public RenderEntity
{
public: //====================================================//

    Cheese_Render(const Entity& entity, const Renderer& renderer);

    //--------------------------------------------------------//

    void integrate(float blend) override;

    void render_depth() override;

    void render_main() override;

private: //===================================================//

    sq::Mesh MESH_Cheese;

    sq::Texture2D TX_Cheese_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Cheese_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Cheese_spec { sq::Texture::Format::RGB8_UN };

    sq::Program PROG_Cheese;

    //--------------------------------------------------------//

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;
};

} // namespace sts
