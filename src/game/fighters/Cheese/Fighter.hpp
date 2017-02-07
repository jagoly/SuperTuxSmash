#pragma once

#include <game/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

class Cheese_Fighter final : public Fighter
{
public:

    //========================================================//

    Cheese_Fighter();
    ~Cheese_Fighter();

    void setup() override;
    void tick() override;
    void integrate() override;
    void render_depth() override;
    void render_main() override;

    //========================================================//

private:

    sq::Mesh MESH_Cheese;

    sq::Texture2D TX_Cheese_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Cheese_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Cheese_spec { sq::Texture::Format::RGB8_UN };

    sq::Shader VS_Cheese { sq::Shader::Stage::Vertex };
    sq::Shader FS_Cheese { sq::Shader::Stage::Fragment };

    //========================================================//

    Mat4F mFinalMatrix;
    Mat3F mNormalMatrix;
};

//============================================================================//

}} // namespace sts::fighters
