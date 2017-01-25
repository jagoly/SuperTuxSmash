#pragma once

#include <game/fighters/Cheese/Fighter.hpp>

#include <render/Fighter.hpp>

namespace sts { namespace fighters {

//============================================================================//

class Cheese_Render final : public RenderFighter
{
public:

    //========================================================//

    Cheese_Render(Renderer& renderer, const Cheese_Fighter& fighter);

    ~Cheese_Render() override;

    //========================================================//

    void render(float progress) override;

    //========================================================//

private:

    //========================================================//

    Vec3F mColour = {0.f, 0.f, 0.f};

    //========================================================//

    struct {

        sq::Mesh MESH_Cheese;

        sq::Texture2D TX_Cheese_diff { sq::Texture::Format::RGB8_UN };
        sq::Texture2D TX_Cheese_norm { sq::Texture::Format::RGB8_SN };
        sq::Texture2D TX_Cheese_spec { sq::Texture::Format::RGB8_UN };

        sq::Shader VS_Simple { sq::Shader::Stage::Vertex };
        sq::Shader FS_Cheese { sq::Shader::Stage::Fragment };

    } stuff;
};

//============================================================================//

}} // namespace sts::fighters
