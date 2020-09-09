#include "fighters/Mario_Fighter.hpp"

#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"
#include "game/ParticleSystem.hpp"

using namespace sts;

//============================================================================//

Mario_Fighter::Mario_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Mario) {}

//============================================================================//

void Mario_Fighter::tick()
{
    base_tick_fighter();

    //--------------------------------------------------------//

    auto& partilcles = world.get_particle_system();

    Vec3F footPosL = Vec3F(get_bone_matrix(get_armature().get_bone_index("LToeN")) * Vec4F(-0.128f, 0.f, 0.13f, 1.f));
    Vec3F footPosR = Vec3F(get_bone_matrix(get_armature().get_bone_index("RToeN")) * Vec4F(+0.128f, 0.f, 0.13f, 1.f));

    // without this, moving into a ledge without walking off looks weird
    if (status.facing == -1) footPosL.x = maths::max(status.position.x, footPosL.x);
    if (status.facing == -1) footPosR.x = maths::max(status.position.x, footPosR.x);
    if (status.facing == +1) footPosL.x = maths::min(status.position.x, footPosL.x);
    if (status.facing == +1) footPosR.x = maths::min(status.position.x, footPosR.x);

    //--------------------------------------------------------//

    if (status.state == State::Brake)
    {
        Emitter emitter;
        emitter.count = 1u;
        emitter.velocity.x = status.velocity.x * 0.5f;
        emitter.baseOpacity = 0.5f;
        emitter.endOpacity = 0.f;
        emitter.endScale = 1.6f;
        emitter.lifetime = { 14u, 16u };
        emitter.baseRadius = { 0.3f, 0.4f };
        emitter.colour = { Vec3F(1.f, 1.f, 1.f) };
        emitter.sprite = "Smoke";

        if (footPosL.y - status.position.y < 0.1f)
        {
            emitter.origin = Vec3F(footPosL.x, status.position.y, footPosL.z);
            partilcles.generate(emitter);
        }

        if (footPosR.y - status.position.y < 0.1f)
        {
            emitter.origin = Vec3F(footPosR.x, status.position.y, footPosR.z);
            partilcles.generate(emitter);
        }
    }

//    if (status.state == State::Landing && mStateProgress == 1u)
//    {
//        Emitter emitter;
//        emitter.velocity.x = status.velocity.x * 0.8f;
//        emitter.baseOpacity = 0.5f;
//        emitter.endOpacity = 0.f;
//        emitter.endScale = 1.8f;
//        emitter.lifetime = { 18u, 24u };
//        emitter.baseRadius = { 0.3f, 0.4f };
//        emitter.colour = { Vec3F(1.f, 1.f, 1.f) };
//        emitter.discIncline = { 0.f, 0.03f };
//        emitter.discOffset = { 0.f, 0.f };
//        emitter.discSpeed = { 0.03f, 0.04f };
//        emitter.sprite = "Smoke";

//        emitter.origin = Vec3F(footPosL.x, current.position.y, footPosL.z);
//        emitter.generate(partilcles, 10u);

//        emitter.origin = Vec3F(footPosR.x, current.position.y, footPosR.z);
//        emitter.generate(partilcles, 10u);
//    }

//    if (status.state == State::Prone && mStateProgress == 1u)
//    {
//        Emitter emitter;
//        emitter.velocity.x = status.velocity.x * 0.6f;
//        emitter.baseOpacity = 0.5f;
//        emitter.endOpacity = 0.f;
//        emitter.endScale = 1.8f;
//        emitter.lifetime = { 22u, 30u };
//        emitter.baseRadius = { 0.3f, 0.4f };
//        emitter.colour = { Vec3F(1.f, 1.f, 1.f) };
//        emitter.discIncline = { 0.f, 0.03f };
//        emitter.discOffset = { 0.f, 0.f };
//        emitter.discSpeed = { 0.08f, 0.12f };
//        emitter.sprite = "Smoke";

//        emitter.origin = Vec3F(current.position, 0.f);
//        emitter.generate(partilcles, 24u);
//    }
}
