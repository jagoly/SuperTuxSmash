#include "game/Emitter.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

void Emitter::from_json(JsonObject json, const sq::Armature& armature)
{
    bone = armature.json_as_bone_index(json["bone"]);

    count = json["count"].as_auto();

    origin = json["origin"].as_auto();
    velocity = json["velocity"].as_auto();

    baseOpacity = json["baseOpacity"].as_auto();
    endOpacity = json["endOpacity"].as_auto();
    endScale = json["endScale"].as_auto();

    lifetime = json["lifetime"].as_auto();
    baseRadius = json["baseRadius"].as_auto();

    ballOffset = json["ballOffset"].as_auto();
    ballSpeed = json["ballSpeed"].as_auto();

    discIncline = json["discIncline"].as_auto();
    discOffset = json["discOffset"].as_auto();
    discSpeed = json["discSpeed"].as_auto();

    if (const auto jColour = json["colour"]; (colour = jColour.as_auto()).empty())
        jColour.throw_with_context("need at least one colour");

    sprite = json["sprite"].as_auto();
}

//============================================================================//

void Emitter::to_json(JsonMutObject json, const sq::Armature& armature) const
{
    json.append("bone", armature.json_from_bone_index(json.document(), bone));

    json.append("count", count);

    json.append("origin", origin);
    json.append("velocity", velocity);

    json.append("baseOpacity", baseOpacity);
    json.append("endOpacity", endOpacity);
    json.append("endScale", endScale);

    json.append("lifetime", lifetime);
    json.append("baseRadius", baseRadius);

    json.append("ballOffset", ballOffset);
    json.append("ballSpeed", ballSpeed);

    json.append("discIncline", discIncline);
    json.append("discOffset", discOffset);
    json.append("discSpeed", discSpeed);

    json.append("colour", colour);

    json.append("sprite", sprite);
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
