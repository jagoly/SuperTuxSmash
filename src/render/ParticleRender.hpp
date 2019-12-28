#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/maths/Builtins.hpp>

#include "game/ParticleSystem.hpp"

#include "render/Renderer.hpp"

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

    Vector<ParticleSetInfo> mParticleSetInfo;
    Vector<ParticleSetInfo> mParticleSetInfoKeep;

    Vector<ParticleVertex> mVertices;

    //--------------------------------------------------------//

    sq::TextureArray2D mTexture;

    sq::FixedBuffer mVertexBuffer;
    sq::VertexArray mVertexArray;

    //--------------------------------------------------------//

    Renderer& renderer;
};

} // namespace sts
