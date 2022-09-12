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
    void generate(const Emitter& emitter, const Entity* entity);

    /// Simulate all particles, then destroy dead particles.
    void update_and_clean();

    //--------------------------------------------------------//

    // temporarily hardcoded list of sprites

    struct SpriteRange { uint16_t first, last; };
    std::map<TinyString, SpriteRange> sprites;

private: //===================================================//

    struct GenerateCall { const Emitter* emitter; const Entity* entity; };

    void impl_generate(const Emitter& emitter, const Entity* entity);

    World& world;

    std::vector<GenerateCall> mGenerateCalls;

    std::vector<ParticleData> mParticles;
};

//============================================================================//

} // namespace sts
