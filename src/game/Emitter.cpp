#include "game/Emitter.hpp"

#include "game/Fighter.hpp"
#include "game/ParticleSystem.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Assert.hpp>
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
    SQASSERT(fighter != nullptr || bone == -1, "bone set without a fighter");

    std::mt19937& rng = gRandomNumberGenerator;

    const auto& [spriteMin, spriteMax] = system.sprites[sprite];

    maths::RandomRange<uint16_t> randSprite { spriteMin, spriteMax };
    maths::RandomRange<uint16_t> randColour { 0u, uint16_t(colour.size() - 1u) };

    maths::RandomRange<Vec3F> randNormal { Vec3F(-1.f), Vec3F(1.f) };
    maths::RandomRange<float> randAngle { 0.f, 1.f };

    const Mat4F boneMatrix = fighter ? fighter->get_bone_matrix(bone) : Mat4F();

    //--------------------------------------------------------//

    for (uint i = 0u; i < count; ++i)
    {
        auto& p = system.mParticles.emplace_back();

        p.progress = 0u;

        p.currentPos = Vec3F();
        p.velocity = Vec3F();

        p.baseOpacity = baseOpacity;
        p.endOpacity = endOpacity;
        p.endScale = endScale;

        p.lifetime = lifetime(rng);
        p.baseRadius = baseRadius(rng);

        p.sprite = randSprite(rng);
        p.colour = colour[uint8_t(randColour(rng))];

        p.friction = 0.1f;

        // apply shapeless launch offset and velocity
        p.currentPos += Vec3F(boneMatrix * Vec4F(origin, 1.f));
        p.velocity += Mat3F(boneMatrix) * velocity;

        // apply ball shape modifiers
        const Vec3F ballDirection = maths::normalize(randNormal(rng));
        p.currentPos += ballDirection * ballOffset(rng);
        p.velocity += ballDirection * ballSpeed(rng);

        // apply disc shape modifiers
        const Vec3F discDirection = maths::rotate_y(maths::rotate_x(Vec3F(0, 0, -1), discIncline(rng)), randAngle(rng));
        p.currentPos += Mat3F(boneMatrix) * discDirection * discOffset(rng);
        p.velocity += Mat3F(boneMatrix) * discDirection * discSpeed(rng);

        // do this once currentPos is ready
        p.previousPos = p.currentPos;
    }
}

//============================================================================//

void Emitter::from_json(const JsonValue& json)
{
    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) throw std::invalid_argument("invalid bone '{}'"_format(jb));
    }
    else bone = -1;

    json.at("origin").get_to(origin);
    json.at("velocity").get_to(velocity);

    json.at("sprite").get_to(sprite);

    json.at("colour").get_to(colour);
    if (colour.empty()) throw std::invalid_argument("no colours defined");

    json.at("baseOpacity").get_to(baseOpacity);
    json.at("endOpacity").get_to(endOpacity);
    json.at("endScale").get_to(endScale);

    json.at("lifetime").get_to(lifetime);
    json.at("baseRadius").get_to(baseRadius);

    json.at("ballOffset").get_to(ballOffset);
    json.at("ballSpeed").get_to(ballSpeed);

    json.at("discIncline").get_to(discIncline);
    json.at("discOffset").get_to(discOffset);
    json.at("discSpeed").get_to(discSpeed);
}

//============================================================================//

void Emitter::to_json(JsonValue& json) const
{
    if (bone == -1) json["bone"] = nullptr;
    else json["bone"] = fighter->get_armature().get_bone_name(bone);

    json["origin"] = origin;
    json["velocity"] = velocity;

    json["sprite"] = sprite;

    json["colour"] = colour;

    json["baseOpacity"] = baseOpacity;
    json["endOpacity"] = endOpacity;
    json["endScale"] = endScale;

    json["lifetime"] = lifetime;
    json["baseRadius"] = baseRadius;

    json["ballOffset"] = ballOffset;
    json["ballSpeed"] = ballSpeed;

    json["discIncline"] = discIncline;
    json["discOffset"] = discOffset;
    json["discSpeed"] = discSpeed;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY;

bool sts::operator==(const Emitter& a, const Emitter& b)
{
    if (a.bone        != b.bone)        return false;
    if (a.origin      != b.origin)      return false;
    if (a.velocity    != b.velocity)    return false;
    if (a.sprite      != b.sprite)      return false;
    if (a.endScale    != b.endScale)    return false;
    if (a.endOpacity  != b.endOpacity)  return false;
    if (a.baseOpacity != b.baseOpacity) return false;
    if (a.lifetime    != b.lifetime)    return false;
    if (a.baseRadius  != b.baseRadius)  return false;
    if (a.colour      != b.colour)      return false;
    if (a.ballOffset  != b.ballOffset)  return false;
    if (a.ballSpeed   != b.ballSpeed)   return false;
    if (a.discIncline != b.discIncline) return false;
    if (a.discOffset  != b.discOffset)  return false;
    if (a.discSpeed   != b.discSpeed)   return false;

    return true;
}

ENABLE_WARNING_FLOAT_EQUALITY;
