#include "game/ParticleSystem.hpp"

#include "game/Emitter.hpp"
#include "game/Entity.hpp"
#include "game/World.hpp"

#include <sqee/maths/Random.hpp>

using namespace sts;

//============================================================================//

ParticleSystem::ParticleSystem(World& world)
    : world(world)
{
    sprites =
    {
        { "Smoke", { 0, 15 } },
        { "Sparkle", { 16, 19 } }
    };
}

//============================================================================//

void ParticleSystem::generate(const Emitter& emitter, const Entity* entity)
{
    // defer generation until after entity matrices have been updated
    mGenerateCalls.push_back({&emitter, entity});
}

void ParticleSystem::impl_generate(const Emitter& emitter, const Entity* entity)
{
    SQASSERT(emitter.bone == -1 || entity != nullptr, "bone set without an entity");

    const auto& [spriteMin, spriteMax] = sprites[emitter.sprite];

    const maths::RandomRange<uint16_t> randSprite { spriteMin, spriteMax };
    const maths::RandomRange<uint16_t> randColour { 0u, uint16_t(emitter.colour.size() - 1u) };
    const maths::RandomRange<Vec3F> randNormal { Vec3F(-1.f), Vec3F(1.f) };

    const Mat4F boneMatrix = entity ? entity->get_model_matrix(emitter.bone) : Mat4F();

    std::mt19937& rng = world.get_rng();

    //--------------------------------------------------------//

    for (uint i = 0u; i < emitter.count; ++i)
    {
        auto& p = mParticles.emplace_back();

        p.progress = 0u;

        p.currentPos = Vec3F();
        p.velocity = Vec3F();

        p.baseOpacity = emitter.baseOpacity;
        p.endOpacity = emitter.endOpacity;
        p.endScale = emitter.endScale;

        p.lifetime = emitter.lifetime(rng);
        p.baseRadius = emitter.baseRadius(rng);

        p.sprite = randSprite(rng);
        p.colour = emitter.colour[uint8_t(randColour(rng))];

        p.friction = 0.1f;

        // apply shapeless launch offset and velocity
        p.currentPos += Vec3F(boneMatrix * Vec4F(emitter.origin, 1.f));
        p.velocity += Mat3F(boneMatrix) * emitter.velocity;

        // apply ball shape modifiers
        // todo: we want this to be evenly distributed, not random
        const Vec3F ballDirection = maths::normalize(randNormal(rng));
        p.currentPos += ballDirection * emitter.ballOffset(rng);
        p.velocity += ballDirection * emitter.ballSpeed(rng);

        // apply disc shape modifiers
        const float discIncline = emitter.discIncline(rng);
        const float discAngle = float(i) / float(emitter.count);
        const Vec3F discDirection = maths::rotate_y(maths::rotate_x(Vec3F(0, 0, -1), discIncline), discAngle);
        p.currentPos += Mat3F(boneMatrix) * discDirection * emitter.discOffset(rng);
        p.velocity += Mat3F(boneMatrix) * discDirection * emitter.discSpeed(rng);
    }
}

//============================================================================//

void ParticleSystem::update_and_clean()
{
    for (const auto& [emitter, entity] : mGenerateCalls)
        impl_generate(*emitter, entity);

    mGenerateCalls.clear();

    const auto predicate = [](const ParticleData& p) { return p.progress == p.lifetime; };
    std::erase_if(mParticles, predicate);

    for (ParticleData& p : mParticles)
    {
        p.progress += 1u;
        p.previousPos = p.currentPos;
        p.currentPos += p.velocity;
        p.velocity -= p.velocity * p.friction;
    }

    const auto compare = [](const ParticleData& a, const ParticleData& b) { return a.currentPos.z > b.currentPos.z; };
    std::sort(mParticles.begin(), mParticles.end(), compare);
}
