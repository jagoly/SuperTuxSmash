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
    const JsonValue json = sq::parse_json_from_file(jsonPath);

    for (const auto& item : json.items())
    {
        SoundEffect& sound = sounds[item.key()];
        sound.from_json(item.value(), world.caches.sounds);
    }
}

//============================================================================//

void EntityDef::initialise_animations(const String& jsonPath)
{
    for (const auto& entry : sq::parse_json_from_file(jsonPath))
    {
        const String& key = entry.front().get_ref<const String&>();

        if (auto [iter, ok] = animations.try_emplace(key); ok)
        {
            try {
                const String path = fmt::format("assets/{}/anims/{}", directory, key);
                iter->second.anim = armature.load_animation_from_file(path);
            }
            catch (const std::exception& ex) {
                sq::log_warning("animation '{}/{}': {}", directory, key, ex.what());
                iter->second.anim = armature.make_null_animation(1u);
                iter->second.fallback = true;
            }

            for (auto flagIter = std::next(entry.begin()); flagIter != entry.end(); ++flagIter)
            {
                const String& str = flagIter->get_ref<const String&>();
                if      (str == "Manual") iter->second.manual = true;
                else if (str == "Loop")   iter->second.loop   = true;
                else if (str == "Motion") iter->second.motion = true;
                else if (str == "Turn")   iter->second.turn   = true;
                else if (str == "Attach") iter->second.attach = true;
                else sq::log_warning("animation '{}/{}': invalid flag '{}'", directory, key, str);
            }

            // animation doesn't have any motion
            if (iter->second.anim.tracks[0].size() == sizeof(Vec3F))
                iter->second.motion = false;
        }
        else sq::log_warning("animation '{}/{}': already loaded", directory, key);
    };
}
