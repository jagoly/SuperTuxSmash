#include "game/ParticleEmitter.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/maths/Random.hpp>

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

// todo: use one RNG for whole program
static std::mt19937 gRandomNumberGenerator { 1337ul };

void ParticleEmitter::reset_random_seed(uint64_t seed)
{
    gRandomNumberGenerator.seed(seed);
}

//============================================================================//

void ParticleEmitter::generate(ParticleSystem& system, uint count)
{
    const size_t genStart = system.mParticles.size();

    std::mt19937& rng = gRandomNumberGenerator;

    //--------------------------------------------------------//

    const auto& [spriteMin, spriteMax] = system.sprites[sprite];

    RandomRange<uint16_t> randSprite { spriteMin, spriteMax };

    for (uint i = 0u; i < count; ++i)
    {
        auto& p = system.mParticles.emplace_back();

        p.previousPos = emitPosition;
        p.currentPos = emitPosition;

        p.progress = 0u;

        p.lifetime = lifetime(rng);
        p.radius = radius(rng);
        p.opacity = opacity(rng);
        p.velocity = emitVelocity + direction * speed(rng);

        p.endScale = endScale;
        p.endOpacity = endOpacity;

        p.sprite = randSprite(rng);

        p.friction = 0.1f;
    }

    //--------------------------------------------------------//

    const Mat3F basis = maths::basis_from_y(direction);

    //--------------------------------------------------------//

    if (auto fixed = std::get_if<FixedColour>(&colour))
    {
        for (size_t i = genStart; i < genStart + count; ++i)
            system.mParticles[i].colour = *fixed;
    }

    if (auto random = std::get_if<RandomColour>(&colour))
    {
        RandomRange<uint8_t> randIndex { 0u, uint8_t(random->size() - 1) };
        for (size_t i = genStart; i < genStart + count; ++i)
            system.mParticles[i].colour = (*random)[randIndex(rng)];
    }

    //--------------------------------------------------------//

    if (auto ball = std::get_if<BallShape>(&shape))
    {
        // not really a spherical distribution, but good enough for now
        RandomRange<Vec3F> randNormal { Vec3F(-1.f), Vec3F(1.f) };

        for (size_t i = genStart; i < genStart + count; ++i)
        {
            // don't care about basis, since we are going in every direction anyway
            const Vec3F dir = maths::normalize(randNormal(rng));
            system.mParticles[i].velocity += dir * ball->speed(rng);
        }
    }

    //--------------------------------------------------------//

    if (auto disc = std::get_if<DiscShape>(&shape))
    {
        RandomRange<float> randAngle { 0.f, 1.f };

        for (size_t i = genStart; i < genStart + count; ++i)
        {
            const Vec3F dir = maths::rotate_y(maths::rotate_x(Vec3F(0.f, 0.f, -1.f), disc->incline(rng)), randAngle(rng));
            system.mParticles[i].velocity += basis * dir * disc->speed(rng);
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
    // todo: figure out what exceptions I should actually throw for missing data

    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) sq::log_warning("Invalid bone name %s", jb);
    }
    else bone = -1;

    json.at("origin").get_to(origin);
    json.at("direction").get_to(direction);
    json.at("sprite").get_to(sprite);
    json.at("endScale").get_to(endScale);
    json.at("endOpacity").get_to(endOpacity);

    json.at("lifetime").get_to(lifetime);
    json.at("radius").get_to(radius);
    json.at("opacity").get_to(opacity);
    json.at("speed").get_to(speed);

    if (auto jColour = json.find("colour.fixed"); jColour != json.end())
        jColour->get_to(colour.emplace<FixedColour>());

    else if (auto jColour = json.find("colour.random"); jColour != json.end())
        jColour->get_to(colour.emplace<RandomColour>());

    else { throw nlohmann::detail::out_of_range::create(403, "missing colour data"); }

    if (auto jShape = json.find("shape.ball"); jShape != json.end())
    {
        BallShape& ref = shape.emplace<BallShape>();
        jShape->at("speed").get_to(ref.speed);
    }

    else if (auto jShape = json.find("shape.disc"); jShape != json.end())
    {
        DiscShape& ref = shape.emplace<DiscShape>();
        jShape->at("incline").get_to(ref.incline);
        jShape->at("speed").get_to(ref.speed);
    }

    else if (auto jShape = json.find("shape.ring"); jShape != json.end())
    {
    }

    else { throw nlohmann::detail::out_of_range::create(403, "missing shape data"); }

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

    json["origin"] = origin;
    json["direction"] = direction;
    json["sprite"] = sprite;
    json["endScale"] = endScale;
    json["endOpacity"] = endOpacity;
    json["lifetime"] = lifetime;
    json["radius"] = radius;
    json["opacity"] = opacity;
    json["speed"] = speed;

    if (auto fixed = std::get_if<FixedColour>(&colour))
        json["colour.fixed"] = *fixed;

    if (auto random = std::get_if<RandomColour>(&colour))
        json["colour.random"] = *random;

    if (auto ball = std::get_if<BallShape>(&shape))
    {
        auto& jShape = json["shape.ball"];
        jShape["speed"] = ball->speed;
    }

    if (auto disc = std::get_if<DiscShape>(&shape))
    {
        auto& jShape = json["shape.disc"];
        jShape["incline"] = disc->incline;
        jShape["speed"] = disc->speed;
    }

    if (auto ring = std::get_if<RingShape>(&shape))
    {
    }
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY;

bool sts::operator!=(const ParticleEmitter::BallShape& a, const ParticleEmitter::BallShape& b)
{
    return a.speed != b.speed;
}

bool sts::operator!=(const ParticleEmitter::DiscShape& a, const ParticleEmitter::DiscShape& b)
{
    return a.incline != b.incline || a.speed != b.speed;
}

bool sts::operator!=(const ParticleEmitter::RingShape& a, const ParticleEmitter::RingShape& b)
{
    return a.deviation != b.deviation || a.incline != b.incline;
}

bool sts::operator==(const ParticleEmitter& a, const ParticleEmitter& b)
{
    if (a.bone       != b.bone)       return false;
    if (a.origin     != b.origin)     return false;
    if (a.direction  != b.direction)  return false;
    if (a.sprite     != b.sprite)     return false;
    if (a.endScale   != b.endScale)   return false;
    if (a.endOpacity != b.endOpacity) return false;
    if (a.lifetime   != b.lifetime)   return false;
    if (a.radius     != b.radius)     return false;
    if (a.opacity    != b.opacity)    return false;
    if (a.speed      != b.speed)      return false;
    if (a.colour     != b.colour)     return false;
    if (a.shape      != b.shape)      return false;

    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY;
