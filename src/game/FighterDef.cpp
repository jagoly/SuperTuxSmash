#include "game/FighterDef.hpp"

#include "game/FighterAction.hpp"
#include "game/FighterState.hpp"
#include "game/HurtBlob.hpp"
#include "game/World.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

FighterDef::FighterDef(World& world, TinyString name)
    : EntityDef(world, fmt::format("fighters/{}", name))
{
    initialise_sounds(fmt::format("assets/{}/Sounds.json", directory));

    initialise_animations("assets/fighters/Animations.json");
    initialise_animations(fmt::format("assets/{}/Animations.json", directory));

    initialise_attributes();
    initialise_hurtblobs();
    initialise_actions();
    initialise_states();
    initialise_articles();
}

FighterDef::~FighterDef()
{
    //if (libraryClass) wrenReleaseHandle(world.vm, libraryClass);
}

//============================================================================//

void FighterDef::initialise_attributes()
{
    const JsonValue json = sq::parse_json_from_file(fmt::format("assets/{}/Attributes.json", directory));

    json.at("walkSpeed")   .get_to(attributes.walkSpeed);
    json.at("dashSpeed")   .get_to(attributes.dashSpeed);
    json.at("airSpeed")    .get_to(attributes.airSpeed);
    json.at("traction")    .get_to(attributes.traction);
    json.at("airMobility") .get_to(attributes.airMobility);
    json.at("airFriction") .get_to(attributes.airFriction);

    json.at("hopHeight")     .get_to(attributes.hopHeight);
    json.at("jumpHeight")    .get_to(attributes.jumpHeight);
    json.at("airHopHeight")  .get_to(attributes.airHopHeight);
    json.at("gravity")       .get_to(attributes.gravity);
    json.at("fallSpeed")     .get_to(attributes.fallSpeed);
    json.at("fastFallSpeed") .get_to(attributes.fastFallSpeed);
    json.at("weight")        .get_to(attributes.weight);

    json.at("walkAnimSpeed") .get_to(attributes.walkAnimSpeed);
    json.at("dashAnimSpeed") .get_to(attributes.dashAnimSpeed);

    json.at("extraJumps")    .get_to(attributes.extraJumps);
    json.at("lightLandTime") .get_to(attributes.lightLandTime);

    json.at("diamondHalfWidth")   .get_to(attributes.diamondHalfWidth);
    json.at("diamondOffsetCross") .get_to(attributes.diamondOffsetCross);
    json.at("diamondOffsetTop")   .get_to(attributes.diamondOffsetTop);
}

//============================================================================//

void FighterDef::initialise_hurtblobs()
{
    const JsonValue json = sq::parse_json_from_file(fmt::format("assets/{}/HurtBlobs.json", directory));

    for (const auto& item : json.items())
    {
        HurtBlobDef& def = hurtBlobs[item.key()];
        def.from_json(item.value(), armature);
    }
}

//============================================================================//

void FighterDef::initialise_actions()
{
    const auto load_action = [this](const String& key)
    {
        if (auto [iter, ok] = actions.try_emplace(key, *this, key); ok)
        {
            try {
                iter->second.load_json_from_file();
                iter->second.load_wren_from_file();
            }
            catch (const std::exception& ex) {
                sq::log_warning("'{}/actions/{}': {}", directory, key, ex.what());
            }
        }
        else sq::log_warning("'{}/actions/{}': already loaded", directory, key);
    };

    for (const auto& entry : sq::parse_json_from_file("assets/fighters/Actions.json"))
        load_action(entry.get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file(fmt::format("assets/{}/Actions.json", directory)))
        load_action(entry.get_ref<const String&>());
}

//============================================================================//

void FighterDef::initialise_states()
{
    const auto load_state = [this](const String& key)
    {
        if (auto [iter, ok] = states.try_emplace(key, *this, key); ok)
        {
            try {
                iter->second.load_wren_from_file();
            }
            catch (const std::exception& ex) {
                sq::log_warning("'{}/states/{}': {}", directory, key, ex.what());
            }
        }
        else sq::log_warning("'{}/states/{}': already loaded", directory, key);
    };

    for (const auto& entry : sq::parse_json_from_file("assets/fighters/States.json"))
        load_state(entry.get_ref<const String&>());

    for (const auto& entry : sq::parse_json_from_file(fmt::format("assets/{}/States.json", directory)))
        load_state(entry.get_ref<const String&>());
}

//============================================================================//

void FighterDef::initialise_articles()
{
    const auto load_article = [this](const String& key, const String& path)
    {
        if (auto [iter, ok] = articles.try_emplace(key, world.load_article_def(path)); !ok)
            sq::log_warning("'{}/articles/{}': already loaded", directory, key);
    };

    const JsonValue json = sq::parse_json_from_file(fmt::format("assets/{}/Articles.json", directory));

    for (const auto& item : json.items())
        load_article(item.key(), item.value().get_ref<const String&>());
}
