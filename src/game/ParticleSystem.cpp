#include <algorithm>

#include <sqee/assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/maths/Random.hpp>

#include "game/ParticleSystem.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

ParticleSystem::ParticleSystem()
{
    sprites =
    {
        { "Smoke", { 0, 15 } },
        { "Sparkle", { 16, 19 } }
    };
}

//============================================================================//

void ParticleSystem::update_and_clean()
{
    //mVerticesX.clear();
    //mVerticesX.reserve(mParticles.size());

    // higher value == further from view
//    const auto compare = [](ParticleData& a, ParticleData& b) { return a.currentPos.z > b.currentPos.z; };
//    std::sort(mParticles.begin(), mParticles.end(), compare);

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

    const auto lambda = [](std::vector<ParticleData>& particles)
    {
        for (ParticleData& p : particles)
        {
            p.previousPos = p.currentPos;
            p.currentPos += p.velocity / 48.f;
            p.velocity -= maths::normalize(p.velocity) * p.friction;
        }

        const auto predicate = [](ParticleData& p) { return ++p.progress == p.lifetime; };
        const auto end = std::remove_if(particles.begin(), particles.end(), predicate);
        particles.erase(end, particles.end());
    };

    lambda(mParticles);
}

void ParticleSystem::compute_vertices(float blend, VertexVec& vertices) const
{
    for (const ParticleData& p : mParticles)
    {
        ParticleVertex& vertex = vertices.emplace_back();
        const float factor = (float(p.progress) + blend) / float(p.lifetime);

        vertex.position = maths::mix(p.previousPos, p.currentPos, blend);
        vertex.radius = maths::mix(p.startRadius, p.endRadius, factor);
        vertex.opacity = maths::mix(p.startOpacity, p.endOpacity, factor);

        vertex.sprite = float(p.sprite);
    }

//    float averageDepth = 0.f;

//    for (const ParticleData& p : mParticles)
//    {
//        ParticleVertex& vertex = vertices.emplace_back();
//        const float factor = (float(p.progress) + blend) / float(p.lifetime);

//        vertex.position = maths::mix(p.previousPos, p.currentPos, blend);
//        vertex.radius = maths::mix(radius.start, radius.finish, factor) * p.scale;
//        vertex.opacity = maths::mix(opacity.start, opacity.finish, factor);

//        // this might be very slow! really we want the size of the texture earlier...
//        vertex.index = float(p.index % maxIndex);

//        averageDepth += vertex.position.z;
//    }

//    return averageDepth / float(mParticles.size());
}
