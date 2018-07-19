#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

#include "game/ParticleSystem.hpp"

#include "render/Renderer.hpp"

namespace sts {

//============================================================================//

class ParticleRender final : sq::NonCopyable
{
public: //====================================================//

    ParticleRender(Renderer& renderer);

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

    sq::TextureArray2D mTexture;

    sq::FixedBuffer mVertexBuffer;
    sq::VertexArray mVertexArray;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
