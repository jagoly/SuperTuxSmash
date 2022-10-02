#pragma once

#include "setup.hpp"

#include <sqee/objects/Texture.hpp>
#include <sqee/vk/SwapBuffer.hpp>

namespace sts {

//============================================================================//

class ParticleRenderer final
{
public: //====================================================//

    ParticleRenderer(Renderer& renderer);

    SQEE_COPY_DELETE(ParticleRenderer)
    SQEE_MOVE_DELETE(ParticleRenderer)

    ~ParticleRenderer();

    void refresh_options_destroy();

    void refresh_options_create();

    void integrate(float blend, const ParticleSystem& system);

    void populate_command_buffer(vk::CommandBuffer cmdbuf);

private: //===================================================//

    Renderer& renderer;

    //--------------------------------------------------------//

    struct ParticleVertex
    {
        Vec3F position;
        float radius;
        uint16_t colour[3];
        uint16_t opacity;
        float layer;
        float padding;
    };

    //--------------------------------------------------------//

    sq::SwapBuffer mVertexBuffer;

    sq::Texture mTexture;

    uint mVertexCount = 0u;
};

//============================================================================//

} // namespace sts
