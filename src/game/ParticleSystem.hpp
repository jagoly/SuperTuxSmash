#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/maths/Builtins.hpp>

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

    using VertexVec = Vector<ParticleVertex>;

    //--------------------------------------------------------//

    ParticleSystem();

    //--------------------------------------------------------//

    struct SpriteRange { uint16_t first, last; };
    std::map<String, SpriteRange> sprites;

    //--------------------------------------------------------//

    void update_and_clean();

    // returns the approx average depth for sorting
    void compute_vertices(float blend, VertexVec& vertices) const;

    const Vector<ParticleData>& get_particles() const { return mParticles; }

private: //===================================================//

    Vector<ParticleData> mParticles;

    //--------------------------------------------------------//

    friend struct ParticleEmitter;
};

//============================================================================//

} // namespace sts
