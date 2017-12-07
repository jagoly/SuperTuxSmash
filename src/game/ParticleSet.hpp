#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

namespace sts {

//============================================================================//

struct ParticleData
{
    Vec3F previousPos;
    float scale;
    Vec3F currentPos;
    uint16_t progress;
    uint16_t lifetime;
    Vec3F velocity;
    uint8_t index;
    char _padding[3];
};

struct ParticleVertex
{
    Vec3F position;
    float radius;
    float opacity;
    float index;
};

//============================================================================//

class ParticleSet final : sq::NonCopyable
{
public: //====================================================//

    using Refs = std::vector<std::reference_wrapper<const ParticleSet>>;

    using VertexVec = std::vector<ParticleVertex>;

    //--------------------------------------------------------//

    string texturePath;
    float friction;

    struct { float start, finish; } radius;
    struct { float start, finish; } opacity;

    //--------------------------------------------------------//

    void update_and_clean();

    // returns the approx average depth for sorting
    float compute_vertices(float blend, uint maxIndex, VertexVec& vertices) const;

    uint16_t get_count() const { return uint16_t(mParticles.size()); }

private: //===================================================//

    std::vector<ParticleData> mParticles;
    std::vector<ParticleVertex> mVertices;

    //--------------------------------------------------------//

    friend struct ParticleGeneratorDisc;
    friend struct ParticleGeneratorColumn;
};

//============================================================================//

struct ParticleGeneratorBase : sq::NonCopyable
{
    virtual ~ParticleGeneratorBase();

    Vec3F emitPosition; ///< Base position for all particles.
    Vec3F emitVelocity; ///< Base velocity for all particles.

    Vec3F direction; ///< Direction of the generated shape.

    struct { uint16_t min, max; } lifetime; ///< Random particle lifetime.
    struct { float    min, max; } scale;    ///< Random particle scale factor.

    /// Generate count particles into a set.
    virtual void generate(ParticleSet& set, uint count) = 0;
};

//============================================================================//

struct ParticleGeneratorDisc final : public ParticleGeneratorBase
{
    struct { float min, max; } incline; ///< Random incline for launch direction.
    struct { float min, max; } speed;   ///< Random magnitude for launch velocity.

    void generate(ParticleSet& set, uint count) override;
};

//============================================================================//

struct ParticleGeneratorColumn final : public ParticleGeneratorBase
{
    struct { float min, max; } deviation; ///< Random distance from column centre.
    struct { float min, max; } speed;     ///< Random magnitude for launch velocity.

    void generate(ParticleSet& set, uint count) override;
};

//============================================================================//

} // namespace sts
