#include "game/Emitter.hpp"

#include "game/Fighter.hpp"
#include "game/ParticleSystem.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

// todo: use one RNG for whole program
static std::mt19937 gRandomNumberGenerator { 1337u };

void Emitter::reset_random_seed(uint_fast32_t seed)
{
    gRandomNumberGenerator.seed(seed);
}

//============================================================================//

void Emitter::generate(ParticleSystem& system, uint count)
{
    const size_t genStart = system.mParticles.size();

    std::mt19937& rng = gRandomNumberGenerator;

    //--------------------------------------------------------//

    const auto& [spriteMin, spriteMax] = system.sprites[sprite];

    maths::RandomRange<uint16_t> randSprite { spriteMin, spriteMax };

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
        maths::RandomRange<uint> randIndex { 0u, uint(random->size() - 1) };
        for (size_t i = genStart; i < genStart + count; ++i)
            system.mParticles[i].colour = (*random)[randIndex(rng)];
    }

    //--------------------------------------------------------//

    if (auto ball = std::get_if<BallShape>(&shape))
    {
        // not really a spherical distribution, but good enough for now
        maths::RandomRange<Vec3F> randNormal { Vec3F(-1.f), Vec3F(1.f) };

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
        maths::RandomRange<float> randAngle { 0.f, 1.f };

        for (size_t i = genStart; i < genStart + count; ++i)
        {
            const Vec3F dir = maths::rotate_y(maths::rotate_x(Vec3F(0.f, 0.f, -1.f), disc->incline(rng)), randAngle(rng));
            system.mParticles[i].velocity += basis * dir * disc->speed(rng);
        }
    }
}

//============================================================================//

void Emitter::from_json(const JsonValue& json)
{
    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) throw std::out_of_range("invalid bone '{}'"_format(jb));
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

    if (auto j = json.find("colour.fixed"); j != json.end())
        j->get_to(colour.emplace<FixedColour>());

    else if (auto j = json.find("colour.random"); j != json.end())
        j->get_to(colour.emplace<RandomColour>());

    else { throw std::out_of_range("missing colour data"); }

    if (auto j = json.find("shape.ball"); j != json.end())
    {
        BallShape& ref = shape.emplace<BallShape>();
        j->at("speed").get_to(ref.speed);
    }

    else if (auto j = json.find("shape.disc"); j != json.end())
    {
        DiscShape& ref = shape.emplace<DiscShape>();
        j->at("incline").get_to(ref.incline);
        j->at("speed").get_to(ref.speed);
    }

    else { throw std::out_of_range("missing shape data"); }
}

//============================================================================//

void Emitter::to_json(JsonValue& json) const
{
    if (bone == -1) json["bone"] = nullptr;
    else json["bone"] = fighter->get_armature().get_bone_name(bone);

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
        json["shape.ball"] = { {"speed", ball->speed} };

    if (auto disc = std::get_if<DiscShape>(&shape))
        json["shape.disc"] = { {"incline", disc->incline}, {"speed", disc->speed} };
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY;

namespace sts {

inline bool operator!=(const Emitter::BallShape& a, const Emitter::BallShape& b)
{
    return a.speed != b.speed;
}

inline bool operator!=(const Emitter::DiscShape& a, const Emitter::DiscShape& b)
{
    return a.incline != b.incline || a.speed != b.speed;
}

} // namespace sts

bool sts::operator==(const Emitter& a, const Emitter& b)
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
