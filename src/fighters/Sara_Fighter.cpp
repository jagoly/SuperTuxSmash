#include "fighters/Sara_Fighter.hpp"

using namespace sts;

//============================================================================//

Sara_Fighter::Sara_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, "Sara")
{
    mLocalDiamond.xNeg = { -0.3f, 0.8f };
    mLocalDiamond.xPos = { +0.3f, 0.8f };
    mLocalDiamond.yNeg = { 0.f, 0.f };
    mLocalDiamond.yPos = { 0.f, 1.4f };
}

//============================================================================//

void Sara_Fighter::tick()
{
    base_tick_fighter();

    base_tick_animation();

    //--------------------------------------------------------//

    ParticleGeneratorDisc generator;

    generator.emitPosition = Vec3F(get_diamond().yNeg, 0.f);
    generator.emitVelocity = Vec3F(get_velocity().x * 0.2f, 0.f, 0.f);
    generator.direction = Vec3F(0.f, 1.f, 0.f);
    generator.scale = { 0.3f, 0.6f };
    generator.lifetime = { 16u, 24u };
    generator.incline = { -0.01f, 0.04f };
    generator.speed = { 1.1f, 1.8f };

//    ParticleGeneratorColumn generator;

//    generator.emitPosition = Vec3F(get_diamond().yNeg, 0.f);
//    generator.emitVelocity = Vec3F(get_velocity() * 0.1f, 0.5f);
//    generator.direction = Vec3F(0.f, 1.f, 0.f);
//    generator.scale = { 0.8f, 1.2f };
//    generator.lifetime = { 48u, 80u };
//    generator.deviation = { 1.3f, 1.5f };
//    generator.speed = { 1.5f, 3.5f };

    mParticleSet.texturePath = "particles/WhitePuff";
    mParticleSet.friction = 0.02f;
    mParticleSet.radius = { 0.5f, 1.f };
    mParticleSet.opacity = { 0.4f, 0.f };

    if (current.state == State::Landing && previous.state != State::Landing)
        generator.generate(mParticleSet, 40u);

    mParticleSet.update_and_clean();
}
