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

    // todo: change to wren expressions
    for (const sq::DrawItem& drawItem : drawItems)
    {
        if (drawItem.condition.empty()) continue;
        if (drawItem.condition == "flinch") continue;
        if (drawItem.condition == "!flinch") continue;
        sq::log_warning("'assets/fighters/{}/Render.json': invalid condition '{}'", name, drawItem.condition);
    }
}

FighterDef::~FighterDef()
{
    //if (libraryClass) wrenReleaseHandle(world.vm, libraryClass);
}

//============================================================================//

void FighterDef::initialise_attributes()
{
    const auto document = JsonDocument::parse_file(fmt::format("assets/{}/Attributes.json", directory));
    const auto json = document.root().as<JsonObject>();

    attributes.walkSpeed   = json["walkSpeed"].as_auto();
    attributes.dashSpeed   = json["dashSpeed"].as_auto();
    attributes.airSpeed    = json["airSpeed"].as_auto();
    attributes.traction    = json["traction"].as_auto();
    attributes.airMobility = json["airMobility"].as_auto();
    attributes.airFriction = json["airFriction"].as_auto();

    attributes.hopHeight     = json["hopHeight"].as_auto();
    attributes.jumpHeight    = json["jumpHeight"].as_auto();
    attributes.airHopHeight  = json["airHopHeight"].as_auto();
    attributes.gravity       = json["gravity"].as_auto();
    attributes.fallSpeed     = json["fallSpeed"].as_auto();
    attributes.fastFallSpeed = json["fastFallSpeed"].as_auto();
    attributes.weight        = json["weight"].as_auto();

    attributes.walkAnimSpeed = json["walkAnimSpeed"].as_auto();
    attributes.dashAnimSpeed = json["dashAnimSpeed"].as_auto();

    attributes.extraJumps    = json["extraJumps"].as_auto();
    attributes.lightLandTime = json["lightLandTime"].as_auto();

    for (const auto [_, jBone] : json["diamondBones"].as<JsonArray>())
        attributes.diamondBones.emplace_back(armature.json_as_bone_index(jBone));

    attributes.diamondMinWidth  = json["diamondMinWidth"].as_auto();
    attributes.diamondMinHeight = json["diamondMinHeight"].as_auto();
}

//============================================================================//

void FighterDef::initialise_hurtblobs()
{
    const auto document = JsonDocument::parse_file(fmt::format("assets/{}/HurtBlobs.json", directory));
    const auto json = document.root().as<JsonObject>();

    for (const auto [key, jBlob] : json)
        hurtBlobs[key].from_json(jBlob.as<JsonObject>(), armature);
}

//============================================================================//

void FighterDef::initialise_actions()
{
    const auto load_action = [this](StringView key)
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

    // standard actions
    {
        const auto document = JsonDocument::parse_file("assets/fighters/Actions.json");
        for (const auto [_, key] : document.root().as<JsonArray>())
            load_action(key.as<StringView>());
    }
    // fighter specific actions
    {
        const auto document = JsonDocument::parse_file(fmt::format("assets/{}/Actions.json", directory));
        for (const auto [_, key] : document.root().as<JsonArray>())
            load_action(key.as<StringView>());
    }
}

//============================================================================//

void FighterDef::initialise_states()
{
    const auto load_state = [this](StringView key)
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

    // standard states
    {
        const auto document = JsonDocument::parse_file("assets/fighters/States.json");
        for (const auto [_, key] : document.root().as<JsonArray>())
            load_state(key.as<StringView>());
    }
    // fighter specific states
    {
        const auto document = JsonDocument::parse_file(fmt::format("assets/{}/States.json", directory));
        for (const auto [_, key] : document.root().as<JsonArray>())
            load_state(key.as<StringView>());
    }
}

//============================================================================//

void FighterDef::initialise_articles()
{
    const auto load_article = [this](StringView key, StringView path)
    {
        if (auto [_, ok] = articles.try_emplace(key, world.load_article_def(String(path))); !ok)
            sq::log_warning("'{}/articles/{}': already loaded", directory, key);
    };

    const auto document = JsonDocument::parse_file(fmt::format("assets/{}/Articles.json", directory));
    const auto json = document.root().as<JsonObject>();

    for (const auto [key, path] : json)
        load_article(key, path.as<StringView>());
}
