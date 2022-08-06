#pragma once

#include "setup.hpp"

#include <sqee/objects/Armature.hpp>
#include <sqee/objects/DrawItem.hpp>

namespace sts {

//============================================================================//

struct EntityDef
{
    EntityDef(World& world, String directory);

    ~EntityDef();

    void initialise_sounds(const String& jsonPath);
    void initialise_animations(const String& jsonPath);

    //--------------------------------------------------------//

    World& world;

    /// Directory containing this entity.
    const String directory;

    /// Short name of this entity.
    const TinyString name;

    //--------------------------------------------------------//

    sq::Armature armature;

    std::map<SmallString, SoundEffect> sounds;
    std::map<SmallString, Animation> animations;

    std::vector<sq::DrawItem> drawItems;
};

//============================================================================//

} // namespace sts
