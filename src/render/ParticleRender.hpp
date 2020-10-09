#pragma once

#include "setup.hpp"

#include "game/ParticleSystem.hpp"

#include <sqee/gl/FixedBuffer.hpp>
#include <sqee/gl/Textures.hpp>
#include <sqee/gl/VertexArray.hpp>

namespace sts {

//============================================================================//

class ParticleRenderer final : sq::NonCopyable
{
public: //====================================================//

    ParticleRenderer(Renderer& renderer);

    void refresh_options();

    //--------------------------------------------------------//

    void swap_sets();

    void integrate_set(float blend, const ParticleSystem& system);

    void render_particles();

private: //===================================================//

    struct ParticleSetInfo
    {
        //TexArrayHandle texture;
        uint16_t startIndex;
        uint16_t vertexCount;
        float averageDepth;
    };

    std::vector<ParticleSetInfo> mParticleSetInfo;
    std::vector<ParticleSetInfo> mParticleSetInfoKeep;

    std::vector<ParticleVertex> mVertices;

    //--------------------------------------------------------//

    sq::TextureArray mTexture;

    sq::FixedBuffer mVertexBuffer;
    sq::VertexArray mVertexArray;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
