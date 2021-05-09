#pragma once

#include "setup.hpp"

#include <random> // mt19937

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

    ParticleSystem();

    //--------------------------------------------------------//

    /// Destroy all particles.
    void clear() { mParticles.clear(); };

    /// Reset the seed used for random number generation.
    void reset_random_seed(uint_fast32_t seed) { mGenerator.seed(seed); };

    /// Access the particle data for this tick.
    const std::vector<ParticleData>& get_particles() const { return mParticles; }

    //--------------------------------------------------------//

    /// Generate particles specified by an Emitter.
    void generate(const Emitter& emitter);

    //--------------------------------------------------------//

    /// Simulate all particles, then destroy dead particles.
    void update_and_clean();

    //--------------------------------------------------------//

    // temporarily hardcoded list of sprites

    struct SpriteRange { uint16_t first, last; };
    std::map<TinyString, SpriteRange> sprites;

private: //===================================================//

    std::mt19937 mGenerator;

    std::vector<ParticleData> mParticles;
};

//============================================================================//

} // namespace sts
