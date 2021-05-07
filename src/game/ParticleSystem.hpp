#pragma once

#include "setup.hpp"

#include <random> // mt19937

namespace sts {

//============================================================================//

struct alignas(16) ParticleData final
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

struct alignas(16) ParticleVertex final
{
    Vec3F position;
    float radius;
    uint16_t colour[3];
    uint16_t opacity;
    float index;
    float padding;
};

static_assert(sizeof(ParticleData) == 80u);
static_assert(sizeof(ParticleVertex) == 32u);

//============================================================================//

class ParticleSystem final : sq::NonCopyable
{
public: //====================================================//

    /// Vertex data that the renderer passes to compute_vertices().
    using VertexVec = std::vector<ParticleVertex>;

    //--------------------------------------------------------//

    ParticleSystem();

    //--------------------------------------------------------//

    /// Destroy all particles.
    void clear() { mParticles.clear(); };

    /// Reset the seed used for random number generation.
    void reset_random_seed(uint_fast32_t seed) { mGenerator.seed(seed); };

    /// Get the number of particles in this system.
    size_t get_vertex_count() const { return mParticles.size(); }

    //--------------------------------------------------------//

    /// Generate particles specified by an Emitter.
    void generate(const Emitter& emitter);

    //--------------------------------------------------------//

    /// Simulate all particles, then destroy dead particles.
    void update_and_clean();

    /// Fill a vector with vertex data for rendering.
    void compute_vertices(float blend, VertexVec& vertices) const;

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
