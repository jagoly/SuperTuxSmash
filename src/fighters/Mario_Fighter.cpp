#include "fighters/Mario_Fighter.hpp"

#include "game/Emitter.hpp"
#include "game/FightWorld.hpp"

using namespace sts;

//============================================================================//

Mario_Fighter::Mario_Fighter(uint8_t index, FightWorld& world)
    : Fighter(index, world, FighterEnum::Mario) {}

//============================================================================//

void Mario_Fighter::tick()
{
    base_tick_fighter();

    //--------------------------------------------------------//

    Emitter emitter;

    emitter.shape = Emitter::DiscShape();

    emitter.direction = Vec3F(0.f, 1.f, 0.f);

    emitter.endScale = 2.f;
    emitter.endOpacity = 0.f;

    emitter.lifetime = { 16u, 24u };
    emitter.radius = { 0.3f, 0.4f };
    emitter.opacity = { 0.4f, 0.5f };

    emitter.colour_fixed() = { 1.f, 1.f, 1.f };

    emitter.shape_disc().incline = { -0.01f, 0.04f };
    emitter.shape_disc().speed = { 1.2f, 2.2f };

    emitter.sprite = "Smoke";

    auto& partilcles = world.get_particle_system();

    const auto get_bone_pos = [&](const char* boneName) -> Vec3F
    {
        const int8_t boneIndex = get_armature().get_bone_index(boneName);
        const Mat4F matrix = get_model_matrix() * maths::transpose(Mat4F(get_bone_matrices()[boneIndex]));
        return Vec3F(matrix[3]);
    };

    /*if (current.state == State::Landing && previous.state != State::Landing)
    {
        emitter.emitPosition = Vec3F(get_position(), 0.f);
        emitter.emitVelocity = Vec3F(get_velocity().x * 0.2f, 0.f, 0.f);

        emitter.generate(partilcles, 20u);
    }*/

    /*else if (current.state == State::Jumping && previous.state == State::PreJump)
    {
        emitter.emitPosition = Vec3F(get_position() - get_velocity() / 48.f, 0.f);
        emitter.emitVelocity = Vec3F(get_velocity().x * 0.2f, 0.f, 0.f);

        emitter.generate(partilcles, 20u);
    }*/

    if (status.state == State::Brake)
    {
        const Vec3F leftFootPos = get_bone_pos("LFootJ");
        const Vec3F rightFootPos = get_bone_pos("RFootJ");

        emitter.emitVelocity = Vec3F(get_velocity().x * 0.5f, 0.f, 0.f);
        emitter.direction = Vec3F(0.f, 1.f, 0.f);

        emitter.shape = Emitter::BallShape();

        emitter.shape_ball().speed = { 0.f, 0.f };

        if (leftFootPos.y - get_position().y < 0.1f)
        {
            emitter.emitPosition = Vec3F(leftFootPos.x, get_position().y, leftFootPos.z);
            emitter.generate(partilcles, 1u);
        }

        if (rightFootPos.y - get_position().y < 0.1f)
        {
            emitter.emitPosition = Vec3F(rightFootPos.x, get_position().y, rightFootPos.z);
            emitter.generate(partilcles, 1u);
        }
    }
}
