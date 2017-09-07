#include "fighters/Sara_Actions.hpp"

//============================================================================//

namespace sts::actions {

//----------------------------------------------------------------------------//

struct Sara_Base : public BaseAction<Sara_Fighter>
{
    using BaseAction<Sara_Fighter>::BaseAction;

    virtual void on_start() override
    {
    }

    virtual void on_collide(HitBlob* blob, Fighter& other) override
    {
        other.apply_hit_basic(*blob);
    }

    virtual void on_finish() override
    {
        world.reset_all_hit_blob_groups(fighter);
        world.disable_all_hit_blobs(fighter);
    }
};

//----------------------------------------------------------------------------//

struct Sara_Neutral_First final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame == 2u) world.enable_hit_blob(blobs["0a"]);
        if (frame == 4u) world.enable_hit_blob(blobs["0b"]);

        if (frame == 6u) world.disable_hit_blob(blobs["0a"]);
        if (frame == 8u) world.disable_hit_blob(blobs["0b"]);

        return frame >= 14u;
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Down final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame ==  4u) world.enable_hit_blob(blobs["0a"]);
        if (frame ==  8u) world.enable_hit_blob(blobs["1a"]);
        if (frame == 12u) world.enable_hit_blob(blobs["2a"]);

        if (frame ==  8u) world.disable_hit_blob(blobs["0a"]);
        if (frame == 12u) world.disable_hit_blob(blobs["1a"]);
        if (frame == 16u) world.disable_hit_blob(blobs["2a"]);

        return frame >= 26u;
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Forward final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame == 4u) world.enable_hit_blob(blobs["0Aa"]);
        if (frame == 7u) world.enable_hit_blob(blobs["0Ca"]);
        if (frame == 7u) world.enable_hit_blob(blobs["0Cb"]);;

        if (frame ==  7u) world.disable_hit_blob(blobs["0Aa"]);
        if (frame == 14u) world.disable_hit_blob(blobs["0Ca"]);
        if (frame == 14u) world.disable_hit_blob(blobs["0Cb"]);

        return frame >= 22u;
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Up final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame == 4u) world.enable_hit_blob(blobs["0Aa"]);
        if (frame == 7u) world.enable_hit_blob(blobs["0Ca"]);
        if (frame == 7u) world.enable_hit_blob(blobs["0Cb"]);;

        if (frame ==  7u) world.disable_hit_blob(blobs["0Aa"]);
        if (frame == 13u) world.disable_hit_blob(blobs["0Ca"]);
        if (frame == 13u) world.disable_hit_blob(blobs["0Cb"]);

        return frame >= 20u;
    }

    using Sara_Base::Sara_Base;
};


//----------------------------------------------------------------------------//

struct Sara_Dash_Attack final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame < 24u)
        {
            // this is arbitary and bad, need to work out a better
            // way to influence fighter movement from actions
            fighter.mVelocity.x += float(fighter.facing) * 0.3f;
        }

        return frame >= 32u;
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

} // namespace sts::actions

//============================================================================//

unique_ptr<sts::Actions> sts::create_actions(FightWorld& world, Sara_Fighter& fighter)
{
    auto actions = std::make_unique<sts::Actions>();

    actions->neutral_first = std::make_unique < actions::Sara_Neutral_First > (world, fighter);
    actions->tilt_down     = std::make_unique < actions::Sara_Tilt_Down     > (world, fighter);
    actions->tilt_forward  = std::make_unique < actions::Sara_Tilt_Forward  > (world, fighter);
    actions->tilt_up       = std::make_unique < actions::Sara_Tilt_Up       > (world, fighter);
    actions->air_back      = std::make_unique<DumbAction>(world, fighter, "Sara Air_Back");
    actions->air_down      = std::make_unique<DumbAction>(world, fighter, "Sara Air_Down");
    actions->air_forward   = std::make_unique<DumbAction>(world, fighter, "Sara Air_Forward");
    actions->air_neutral   = std::make_unique<DumbAction>(world, fighter, "Sara Air_Neutral");
    actions->air_up        = std::make_unique<DumbAction>(world, fighter, "Sara Air_Up");
    actions->dash_attack   = std::make_unique < actions::Sara_Dash_Attack   > (world, fighter);

    actions->load_json("Sara");

    return actions;
}
