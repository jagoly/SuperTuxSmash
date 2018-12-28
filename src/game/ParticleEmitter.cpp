#include "game/ParticleEmitter.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/maths/Random.hpp>

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

void ParticleEmitter::generate(ParticleSystem& system, uint count)
{
    const size_t genStart = system.mParticles.size();

    //--------------------------------------------------------//

    static std::mt19937 gen(1337);

    // todo: sprite range map thing

    //maths::RandomScalar<uint16_t> randSprite   { sprite.min, sprite.max };
    maths::RandomScalar<uint16_t> randSprite   { 0u, 1u };
    maths::RandomScalar<uint16_t> randLifetime { lifetime.min, lifetime.max };
    maths::RandomScalar<float>    randRadius   { radius.min, radius.max };
    maths::RandomVector<3, float> randColour   { colour.min, colour.max };
    maths::RandomScalar<float>    randOpacity  { opacity.min, opacity.max };

    for (uint i = 0u; i < count; ++i)
    {
        auto& p = system.mParticles.emplace_back();

        p.previousPos = emitPosition;
        p.currentPos = emitPosition;

        p.progress = 0u;

        p.lifetime = randLifetime(gen);
        p.radius = randRadius(gen);
        p.colour = randColour(gen);
        p.opacity = randOpacity(gen);

        p.endScale = endScale;
        p.endOpacity = endOpacity;

        p.sprite = randSprite(gen);

        p.friction = 0.1f;
    }

    //--------------------------------------------------------//

    const Mat3F basis = maths::basis_from_y(direction);

    //--------------------------------------------------------//

    if (auto disc = std::get_if<DiscShape>(&shape))
    {
        maths::RandomScalar<float> randAngle   { 0.0f, 1.0f };
        maths::RandomScalar<float> randIncline { disc->incline.min, disc->incline.max };
        maths::RandomScalar<float> randSpeed   { disc->speed.min, disc->speed.max };

        for (size_t i = genStart; i < genStart + count; ++i)
        {
            ParticleData& p = system.mParticles[i];

            p.velocity = maths::rotate_x(Vec3F(0.f, 0.f, -1.f), randIncline(gen));
            p.velocity = maths::rotate_y(p.velocity, randAngle(gen));
            p.velocity = emitVelocity + basis * p.velocity * randSpeed(gen);
        }
    }

    //--------------------------------------------------------//

    if (auto ball = std::get_if<BallShape>(&shape))
    {
        maths::RandomVector<3, float> randVector { Vec3F(-1.f), Vec3F(1.f) };
        maths::RandomScalar<float> randSpeed { ball->speed.min, ball->speed.max };

        for (size_t i = genStart; i < genStart + count; ++i)
        {
            ParticleData& p = system.mParticles[i];

            while (true)
            {
                p.velocity = randVector(gen);
                /*if (maths::length_squared(p.velocity) < 1.0f) */break;
            }

            p.velocity =  p.velocity * randSpeed(gen);
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

void ParticleEmitter::from_json(const sq::JsonValue& json)
{
    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) sq::log_warning("Invalid bone name %s", jb);
    }
    else bone = -1;

    json.at("direction").get_to(direction);
    json.at("origin").get_to(origin);

    json.at("endScale").get_to(endScale);
    json.at("endOpacity").get_to(endOpacity);

    json.at("lifetime.min").get_to(lifetime.min);
    json.at("lifetime.max").get_to(lifetime.max);

    json.at("radius.min").get_to(radius.min);
    json.at("radius.max").get_to(radius.max);

    json.at("colour.min").get_to(colour.min);
    json.at("colour.max").get_to(colour.max);

    json.at("opacity.min").get_to(opacity.min);
    json.at("opacity.max").get_to(opacity.max);

    json.at("sprite").get_to(sprite);

    const String& shapeName = json.at("shape");

    if (shapeName == "disc")
    {
        DiscShape& ref = shape.emplace<DiscShape>();

        json.at("incline.min").get_to(ref.incline.min);
        json.at("incline.max").get_to(ref.incline.max);

        json.at("speed.min").get_to(ref.speed.min);
        json.at("speed.max").get_to(ref.speed.max);
    }

    else if (shapeName == "column")
    {

    }

    else if (shapeName == "ball")
    {
        BallShape& ref = shape.emplace<BallShape>();

        json.at("speed.min").get_to(ref.speed.min);
        json.at("speed.max").get_to(ref.speed.max);
    }

    else SQASSERT(false, "");

    return;
}

//============================================================================//

void ParticleEmitter::to_json(JsonValue& json) const
{
    if (bone != -1)
    {
        const auto boneName = fighter->get_armature().get_bone_name(bone);
        if (boneName.empty()) sq::log_warning("Invalid bone index %d", bone);
        json["bone"] = boneName;
    }
    else json["bone"] = nullptr;

    json["direction"] = direction;
    json["origin"] = origin;
    json["endScale"] = endScale;
    json["endOpacity"] = endOpacity;
    json["lifetime.min"] = lifetime.min;
    json["lifetime.max"] = lifetime.max;
    json["radius.min"] = radius.min;
    json["radius.max"] = radius.max;
    json["colour.min"] = colour.min;
    json["colour.max"] = colour.max;
    json["opacity.min"] = opacity.min;
    json["opacity.max"] = opacity.max;
    json["sprite"] = sprite;

    if (auto disc = std::get_if<DiscShape>(&shape))
    {
        json["shape"] = "disc";

        json["incline.min"] = disc->incline.min;
        json["incline.max"] = disc->incline.max;

        json["speed.min"] = disc->speed.min;
        json["speed.max"] = disc->speed.max;
    }

    if (auto ball = std::get_if<BallShape>(&shape))
    {
        json["shape"] = "ball";

        json["speed.min"] = ball->speed.min;
        json["speed.max"] = ball->speed.max;
    }
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY;

bool sts::operator!=(const ParticleEmitter::DiscShape& a, const ParticleEmitter::DiscShape& b)
{
    return a.incline.min != b.incline.min || a.incline.max != b.incline.max
        || a.speed.min != b.speed.min || a.speed.max != b.speed.max;
}

bool sts::operator!=(const ParticleEmitter::ColumnShape& a, const ParticleEmitter::ColumnShape& b)
{
    return a.deviation.min != b.deviation.min || a.deviation.max != b.deviation.max
        || a.speed.min != b.speed.min || a.speed.max != b.speed.max;
}

bool sts::operator!=(const ParticleEmitter::BallShape& a, const ParticleEmitter::BallShape& b)
{
    return a.speed.min != b.speed.min || a.speed.max != b.speed.max;
}

bool sts::operator==(const ParticleEmitter& a, const ParticleEmitter& b)
{
    if (a.origin    != b.origin)    return false;
    if (a.bone      != b.bone)      return false;
    if (a.direction != b.direction) return false;
    if (a.shape     != b.shape)     return false;
    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY;
