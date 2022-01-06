#pragma once

#include "setup.hpp"

namespace sts {

//============================================================================//

struct ParticleData final
{
    Vec3F previousPos;
    uint16_t progress;
    uint16_t lifetime;
    Vec3F currentPos;
    float baseRadius;
    Vec3F velocity;
    float endScale;
    Vec3F colour;
    float baseOpacity;
    float endOpacity;
    float friction;
    uint16_t sprite;
};

//============================================================================//

class ParticleSystem final : sq::NonCopyable
{
public: //====================================================//

    ParticleSystem(World& world);

    //--------------------------------------------------------//

    /// Destroy all particles.
    void clear() { mParticles.clear(); };

    /// Access the particle data for this tick.
    const std::vector<ParticleData>& get_particles() const { return mParticles; }

    //--------------------------------------------------------//

    /// Generate particles specified by an Emitter.
    void generate(const Emitter& emitter);

    /// Simulate all particles, then destroy dead particles.
    void update_and_clean();

    //--------------------------------------------------------//

    // temporarily hardcoded list of sprites

    struct SpriteRange { uint16_t first, last; };
    std::map<TinyString, SpriteRange> sprites;

private: //===================================================//

    World& world;

    std::vector<ParticleData> mParticles;
};

//============================================================================//

} // namespace sts
