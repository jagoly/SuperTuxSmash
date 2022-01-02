#include "game/Emitter.hpp"

#include "game/Fighter.hpp"

#include <sqee/debug/Assert.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void Emitter::from_json(const JsonValue& json)
{
    SQASSERT(fighter != nullptr, "");

    bone = fighter->bone_from_json(json.at("bone"));

    json.at("count").get_to(count);

    json.at("origin").get_to(origin);
    json.at("velocity").get_to(velocity);

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

    json.at("colour").get_to(colour);
    if (colour.empty()) SQEE_THROW("no colours defined");

    json.at("sprite").get_to(sprite);
}

//============================================================================//

void Emitter::to_json(JsonValue& json) const
{
    SQASSERT(fighter != nullptr, "");

    json["bone"] = fighter->bone_to_json(bone);

    json["count"] = count;

    json["origin"] = origin;
    json["velocity"] = velocity;

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

    json["colour"] = colour;

    json["sprite"] = sprite;
}

//============================================================================//

DISABLE_WARNING_FLOAT_EQUALITY()

bool Emitter::operator==(const Emitter& other) const
{
    return bone == other.bone &&
           count == other.count &&
           origin == other.origin &&
           velocity == other.velocity &&
           baseOpacity == other.baseOpacity &&
           endOpacity == other.endOpacity &&
           endScale == other.endScale &&
           lifetime == other.lifetime &&
           baseRadius == other.baseRadius &&
           ballOffset == other.ballOffset &&
           ballSpeed == other.ballSpeed &&
           discIncline == other.discIncline &&
           discOffset == other.discOffset &&
           discSpeed == other.discSpeed &&
           colour == other.colour &&
           sprite == other.sprite;
}

ENABLE_WARNING_FLOAT_EQUALITY()
