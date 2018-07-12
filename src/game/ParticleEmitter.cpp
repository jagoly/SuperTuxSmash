#include <algorithm>

#include <sqee/assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/maths/Random.hpp>

#include "game/ParticleEmitter.hpp"

#include <sqee/debug/Misc.hpp>

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

void ParticleEmitter::generate(ParticleSet& set, uint count)
{
    static std::mt19937 gen(1337);

    maths::RandomScalar<uint8_t>  randIndex    { 0u, 63u };
    maths::RandomScalar<uint16_t> randLifetime { lifetime.min, lifetime.max };
    maths::RandomScalar<float>    randScale    { scale.min, scale.max };

    const Mat3F basis = maths::basis_from_y(direction);

    for (uint i = 0u; i < count; ++i)
    {
        auto& p = set.mParticles.emplace_back();

        p.previousPos = emitPosition;
        p.currentPos = emitPosition;

        p.progress = 0u;
        p.lifetime = randLifetime(gen);

        const float scaleFactor = randScale(gen);
        p.startRadius = radius.start * scaleFactor;
        p.endRadius = radius.end * scaleFactor;;

        p.startOpacity = opacity.start;
        p.endOpacity = opacity.end;

        p.index = randIndex(gen);
    }

    //--------------------------------------------------------//

    if (auto disc = std::get_if<DiscShape>(&shape))
    {
        maths::RandomScalar<float> randAngle   { 0.0f, 1.0f };
        maths::RandomScalar<float> randIncline { disc->incline.min, disc->incline.max };
        maths::RandomScalar<float> randSpeed   { disc->speed.min, disc->speed.max };

        for (auto& p : set.mParticles)
        {
            p.velocity = maths::rotate_x(Vec3F(0.f, 0.f, -1.f), randIncline(gen));
            p.velocity = maths::rotate_y(p.velocity, randAngle(gen));
            p.velocity = emitVelocity + basis * p.velocity * randSpeed(gen);
        }
    }
}

//void ParticleGeneratorColumn::generate(ParticleSet& set, uint count)
//{
//    static std::mt19937 gen(1337);

//    maths::RandomScalar<uint8_t>  randIndex    { 0u, 63u };
//    maths::RandomScalar<uint16_t> randLifetime { lifetime.min, lifetime.max };
//    maths::RandomScalar<float>    randScale    { scale.min, scale.max };

//    maths::RandomScalar<float> randAngle     { 0.0f, 1.0f };
//    maths::RandomScalar<float> randDeviation { deviation.min, deviation.max };
//    maths::RandomScalar<float> randSpeed     { speed.min, speed.max };

//    const Mat3F basis = maths::basis_from_y(direction);

//    for (uint i = 0u; i < count; ++i)
//    {
//        auto& data = set.mParticles.emplace_back();

//        data.previousPos = emitPosition + basis * maths::rotate_y(Vec3F(0.f, 0.f, randDeviation(gen)), randAngle(gen));
//        data.scale = randScale(gen);
//        data.currentPos = data.previousPos;
//        data.progress = 0u;
//        data.lifetime = randLifetime(gen);
//        data.velocity = emitVelocity + direction * randSpeed(gen);
//        data.index = randIndex(gen);
//    }
//}

//============================================================================//

void ParticleEmitter::load_from_json(const sq::JsonValue& json)
{
    direction = json.at("direction");

    lifetime.min = json.at("minLifetime");
    lifetime.max = json.at("maxLifetime");

    scale.min = json.at("minScale");
    scale.max = json.at("maxScale");

    radius.start = json.at("startRadius");
    radius.end = json.at("endRadius");

    opacity.start = json.at("startOpacity");
    opacity.end = json.at("endOpacity");

    const string& shapeName = json.at("shape");

    if (shapeName == "disc")
    {
        DiscShape& ref = shape.emplace<DiscShape>();

        ref.incline.min = json.at("minIncline");
        ref.incline.max = json.at("maxIncline");

        ref.speed.min = json.at("minSpeed");
        ref.speed.max = json.at("maxSpeed");
    }

    else if (shapeName == "column")
    {

    }

    else SQASSERT(false, "");

    return;
}
