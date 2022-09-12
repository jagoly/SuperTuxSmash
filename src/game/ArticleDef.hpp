#pragma once

#include "setup.hpp"

#include "game/EntityDef.hpp"

namespace sts {

//============================================================================//

struct ArticleDef final : EntityDef
{
    ArticleDef(World& world, String directory);

    ~ArticleDef();

    //--------------------------------------------------------//

    WrenHandle* scriptClass = nullptr;

    std::map<TinyString, HitBlobDef> blobs;
    std::map<TinyString, VisualEffectDef> effects;
    std::map<TinyString, Emitter> emitters;

    // todo: find a way to move this to the editor
    String wrenSource;

    //--------------------------------------------------------//

    void load_json_from_file();

    void load_wren_from_file();

    void interpret_module();
};

//============================================================================//

} // namespace sts
