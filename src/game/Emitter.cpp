#include "game/Emitter.hpp"

#include "game/Fighter.hpp"
#include "game/ParticleSystem.hpp"

#include <sqee/debug/Logging.hpp>
#include <sqee/debug/Assert.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void Emitter::from_json(const JsonValue& json)
{
    if (auto& jb = json.at("bone"); jb.is_null() == false)
    {
        bone = fighter->get_armature().get_bone_index(jb);
        if (bone == -1) throw std::invalid_argument("invalid bone '{}'"_format(jb));
    }
    else bone = -1;

    json.at("count").get_to(count);

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

    json["count"] = count;

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

DISABLE_WARNING_FLOAT_EQUALITY()

bool sts::operator==(const Emitter& a, const Emitter& b)
{
    if (a.bone        != b.bone)        return false;
    if (a.count       != b.count)       return false;
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

ENABLE_WARNING_FLOAT_EQUALITY()
