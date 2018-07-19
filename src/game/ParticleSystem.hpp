#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>
#include <sqee/misc/Json.hpp>

namespace sts {

//============================================================================//

struct ParticleData
{
    Vec3F previousPos;
    Vec3F currentPos;
    Vec3F velocity;
    uint16_t progress;
    uint16_t lifetime;
    float startRadius;
    float endRadius;
    float startOpacity;
    float endOpacity;
    float friction;
    uint16_t sprite;
    char _padding[2];
};

struct ParticleVertex
{
    Vec3F position;
    float radius;
    float opacity;
    float sprite;
};

//============================================================================//

class ParticleSystem final : sq::NonCopyable
{
public: //====================================================//

    using VertexVec = std::vector<ParticleVertex>;

    //--------------------------------------------------------//

    ParticleSystem();

    //--------------------------------------------------------//

    struct SpriteRange { uint16_t first, last; };
    std::map<string, SpriteRange> sprites;

    //--------------------------------------------------------//

    void update_and_clean();

    // returns the approx average depth for sorting
    void compute_vertices(float blend, VertexVec& vertices) const;

    const std::vector<ParticleData>& get_particles() const { return mParticles; }

    //uint16_t get_count() const { return uint16_t(mParticles.size()); }

private: //===================================================//

    std::vector<ParticleData> mParticles;

    //--------------------------------------------------------//

    friend struct ParticleEmitter;
};

//============================================================================//

} // namespace sts
