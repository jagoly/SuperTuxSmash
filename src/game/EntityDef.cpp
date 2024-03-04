#include "game/EntityDef.hpp"

#include "game/SoundEffect.hpp"
#include "game/World.hpp"

#include "render/AnimPlayer.hpp"

#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

EntityDef::EntityDef(World& world, String directory)
    : world(world), directory(directory)
    , name(StringView(directory).substr(directory.rfind('/') + 1))
    , armature(fmt::format("assets/{}/Armature.json", directory))
{
    drawItems = sq::DrawItem::load_from_json (
        fmt::format("assets/{}/Render.json", directory), armature,
        world.caches.meshes, world.caches.pipelines, world.caches.textures
    );
}

EntityDef::~EntityDef() = default;

//============================================================================//

void EntityDef::initialise_sounds(const String& jsonPath)
{
    const auto document = JsonDocument::parse_file(jsonPath);

    for (const auto [key, jSound] : document.root().as<JsonObject>() | views::json_as<JsonObject>)
        sounds[key].from_json(jSound, world.caches.sounds);
}

//============================================================================//

void EntityDef::initialise_animations(const String& jsonPath)
{
    const auto document = JsonDocument::parse_file(jsonPath);

    for (const auto [key, jFlags] : document.root().as<JsonObject>() | views::json_as<JsonArray>)
    {
        if (auto [iter, ok] = animations.try_emplace(key); ok)
        {
            try
            {
                const auto animPath = fmt::format("assets/{}/anims/{}", directory, key);
                iter->second.anim = armature.load_animation_from_file(animPath);
            }
            catch (const std::exception& ex)
            {
                sq::log_warning("animation '{}/{}': {}", directory, key, ex.what());
                iter->second.anim = armature.make_null_animation(1u);
                iter->second.fallback = true;
            }

            for (const auto [_, flag] : jFlags | views::json_as<StringView>)
            {
                if      (flag == "Manual") iter->second.manual = true;
                else if (flag == "Loop")   iter->second.loop   = true;
                else if (flag == "Motion") iter->second.motion = true;
                else if (flag == "Turn")   iter->second.turn   = true;
                else if (flag == "Attach") iter->second.attach = true;
                else sq::log_warning("animation '{}/{}': invalid flag '{}'", directory, key, flag);
            }

            // animation doesn't have any motion
            if (iter->second.anim.tracks[0].size() == sizeof(Vec3F))
                iter->second.motion = false;
        }
        else sq::log_warning("animation '{}/{}': already loaded", directory, key);
    };
}
