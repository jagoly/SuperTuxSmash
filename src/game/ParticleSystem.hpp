#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/maths/Builtins.hpp>

#include <sqee/misc/Json.hpp>

namespace sts {

//============================================================================//

struct ParticleData
{
    Vec3F previousPos;
    uint16_t progress;
    uint16_t lifetime;
    Vec3F currentPos;
    float radius;
    Vec3F velocity;
    float endScale;
    Vec3F colour;
    float opacity;
    float endOpacity;
    float friction;
    uint16_t sprite;
    char _padding[6];
};

struct ParticleVertex
{
    Vec3F position;
    float radius;
    uint16_t colour[3];
    uint16_t opacity;
    float misc;
    float index;
};

static_assert(sizeof(ParticleData) == 80u);
static_assert(sizeof(ParticleVertex) == 32u);

//============================================================================//

class ParticleSystem final : sq::NonCopyable
{
public: //====================================================//

    using VertexVec = Vector<ParticleVertex>;

    //--------------------------------------------------------//

    ParticleSystem();

    /// Immediately destroy all particles.
    void clear();

    //--------------------------------------------------------//

    struct SpriteRange { uint16_t first, last; };
    std::map<String, SpriteRange> sprites;

    //--------------------------------------------------------//

    /// Simulate all particles, then destroy dead particles.
    void update_and_clean();

    void compute_vertices(float blend, VertexVec& vertices) const;

    const Vector<ParticleData>& get_particles() const { return mParticles; }

private: //===================================================//

    Vector<ParticleData> mParticles;

    //--------------------------------------------------------//

    friend struct ParticleEmitter;
};

//============================================================================//

} // namespace sts
