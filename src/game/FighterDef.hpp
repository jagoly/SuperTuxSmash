#pragma once

#include "setup.hpp"

#include "game/EntityDef.hpp"

namespace sts {

//============================================================================//

struct FighterDef final : EntityDef
{
    /// Stats that don't change during a game.
    struct Attributes
    {
        float walkSpeed   = 0.1f;
        float dashSpeed   = 0.15f;
        float airSpeed    = 0.1f;
        float traction    = 0.005f;
        float airMobility = 0.008f;
        float airFriction = 0.002f;

        float hopHeight     = 1.5f;
        float jumpHeight    = 3.5f;
        float airHopHeight  = 3.5f;
        float gravity       = 0.01f;
        float fallSpeed     = 0.15f;
        float fastFallSpeed = 0.24f;
        float weight        = 100.f;

        float walkAnimSpeed = 0.1f;
        float dashAnimSpeed = 0.15f;

        uint extraJumps    = 2u;
        uint lightLandTime = 4u;

        float diamondHalfWidth   = 0.4f;
        float diamondOffsetCross = 1.4f;
        float diamondOffsetTop   = 0.8f;
    };

    //--------------------------------------------------------//

    FighterDef(World& world, TinyString name);

    ~FighterDef();

    void initialise_attributes();
    void initialise_hurtblobs();
    void initialise_actions();
    void initialise_states();
    void initialise_articles();

    //--------------------------------------------------------//

    Attributes attributes;

    std::map<TinyString, HurtBlobDef> hurtBlobs;
    std::map<SmallString, FighterActionDef> actions;
    std::map<TinyString, FighterStateDef> states;

    std::map<TinyString, std::reference_wrapper<ArticleDef>> articles;

    //WrenHandle* libraryClass = nullptr;
};

//============================================================================//

} // namespace sts
