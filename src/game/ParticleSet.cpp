#include <algorithm>

#include <sqee/assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/maths/Random.hpp>

#include "game/ParticleSet.hpp"

#include <sqee/debug/Misc.hpp>

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

ParticleGeneratorBase::~ParticleGeneratorBase() = default;

void ParticleGeneratorDisc::generate(ParticleSet& set, uint count)
{
    static std::mt19937 gen(1337);

    maths::RandomScalar<uint8_t>  randIndex    { 0u, 63u };
    maths::RandomScalar<uint16_t> randLifetime { lifetime.min, lifetime.max };
    maths::RandomScalar<float>    randScale    { scale.min, scale.max };

    maths::RandomScalar<float> randAngle   { 0.0f, 1.0f };
    maths::RandomScalar<float> randIncline { incline.min, incline.max };
    maths::RandomScalar<float> randSpeed   { speed.min, speed.max };

    const Mat3F basis = maths::basis_from_y(direction);

    for (uint i = 0u; i < count; ++i)
    {
        auto& data = set.mParticles.emplace_back();

        data.currentPos = data.previousPos = emitPosition;
        data.scale = randScale(gen);
        data.progress = 0u;
        data.lifetime = randLifetime(gen);
        data.velocity = maths::rotate_x(Vec3F(0.f, 0.f, -1.f), randIncline(gen));
        data.velocity = maths::rotate_y(data.velocity, randAngle(gen));
        data.velocity = emitVelocity + basis * data.velocity * randSpeed(gen);
        data.index = randIndex(gen);
    }
}

void ParticleGeneratorColumn::generate(ParticleSet& set, uint count)
{
    static std::mt19937 gen(1337);

    maths::RandomScalar<uint8_t>  randIndex    { 0u, 63u };
    maths::RandomScalar<uint16_t> randLifetime { lifetime.min, lifetime.max };
    maths::RandomScalar<float>    randScale    { scale.min, scale.max };

    maths::RandomScalar<float> randAngle     { 0.0f, 1.0f };
    maths::RandomScalar<float> randDeviation { deviation.min, deviation.max };
    maths::RandomScalar<float> randSpeed     { speed.min, speed.max };

    const Mat3F basis = maths::basis_from_y(direction);

    for (uint i = 0u; i < count; ++i)
    {
        auto& data = set.mParticles.emplace_back();

        data.previousPos = emitPosition + basis * maths::rotate_y(Vec3F(0.f, 0.f, randDeviation(gen)), randAngle(gen));
        data.scale = randScale(gen);
        data.currentPos = data.previousPos;
        data.progress = 0u;
        data.lifetime = randLifetime(gen);
        data.velocity = emitVelocity + direction * randSpeed(gen);
        data.index = randIndex(gen);
    }
}

void ParticleSet::update_and_clean()
{
    mVertices.clear();
    mVertices.reserve(mParticles.size());

    // higher value == further from view
    const auto compare = [](ParticleData& a, ParticleData& b) { return a.currentPos.z > b.currentPos.z; };
    std::sort(mParticles.begin(), mParticles.end(), compare);

//    for (ParticleData& p : mParticles)
//    {
//        p.previousPos = p.currentPos;

//        ParticleVertex& vertex = mVertices.emplace_back();
//        const float factor = float(p.progress) / float(p.lifetime);

//        p.position += p.velocity / 48.f;
//        p.velocity -= maths::normalize(p.velocity) * friction;

//        vertex.position = p.position;
//        vertex.radius = maths::mix(radius.start, radius.finish, factor);
//        vertex.opacity = maths::mix(opacity.start, opacity.finish, factor);
//        vertex.index = p.index;

//        mAverageDepth += vertex.position.z;
//    }

    for (ParticleData& p : mParticles)
    {
        p.previousPos = p.currentPos;
        p.currentPos += p.velocity / 48.f;
        p.velocity -= maths::normalize(p.velocity) * friction;
    }

    const auto predicate = [](ParticleData& p) { return ++p.progress == p.lifetime; };
    const auto end = std::remove_if(mParticles.begin(), mParticles.end(), predicate);
    mParticles.erase(end, mParticles.end());
}

float ParticleSet::compute_vertices(float blend, uint maxIndex, VertexVec& vertices) const
{
    float averageDepth = 0.f;

    for (const ParticleData& p : mParticles)
    {
        ParticleVertex& vertex = vertices.emplace_back();
        const float factor = (float(p.progress) + blend) / float(p.lifetime);

        vertex.position = maths::mix(p.previousPos, p.currentPos, blend);
        vertex.radius = maths::mix(radius.start, radius.finish, factor) * p.scale;
        vertex.opacity = maths::mix(opacity.start, opacity.finish, factor);

        // this might be very slow! really we want the size of the texture earlier...
        vertex.index = float(p.index % maxIndex);

        averageDepth += vertex.position.z;
    }

    return averageDepth / float(mParticles.size());
}
