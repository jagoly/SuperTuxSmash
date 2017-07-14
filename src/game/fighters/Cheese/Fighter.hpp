#pragma once

#include <game/Renderer.hpp>
#include <game/Fighter.hpp>

//============================================================================//

namespace sts::fighters {

class Cheese_Fighter final : public Fighter
{
public: //====================================================//

    Cheese_Fighter(Game& game, Controller& controller);
    ~Cheese_Fighter();

    //--------------------------------------------------------//

    void setup() override;

    void tick() override;

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

} // namespace sts::fighters
