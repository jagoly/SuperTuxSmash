#pragma once

#include <game/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

class Sara_Fighter final : public Fighter
{
public:

    //========================================================//

    Sara_Fighter();
    ~Sara_Fighter();

    void setup() override;
    void tick() override;
    void render() override;

    //========================================================//

private:

    sq::Mesh MESH_Sara;

    sq::Texture2D TX_Main_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Main_spec { sq::Texture::Format::R8_UN };

    sq::Texture2D TX_Hair_diff { sq::Texture::Format::RGB8_UN };
    sq::Texture2D TX_Hair_norm { sq::Texture::Format::RGB8_SN };
    sq::Texture2D TX_Hair_mask { sq::Texture::Format::R8_UN };

    sq::Shader VS_Simple { sq::Shader::Stage::Vertex };
    sq::Shader FS_Main { sq::Shader::Stage::Fragment };
    sq::Shader FS_Hair { sq::Shader::Stage::Fragment };
};

//============================================================================//

}} // namespace sts::fighters
