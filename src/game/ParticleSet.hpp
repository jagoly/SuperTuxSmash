#pragma once

#include <variant>

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

    using VertexVec = std::vector<ParticleVertex>;

    //--------------------------------------------------------//

    string texturePath;

    void update_and_clean();

    // returns the approx average depth for sorting
    void compute_vertices(float blend, uint maxIndex, VertexVec& vertices) const;

    uint16_t get_count() const { return uint16_t(mParticles.size()); }

private: //===================================================//

    std::vector<ParticleData> mParticles;

    //--------------------------------------------------------//

    friend struct ParticleEmitter;
};

//============================================================================//

} // namespace sts
