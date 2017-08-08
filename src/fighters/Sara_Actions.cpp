#include "fighters/Sara_Actions.hpp"

//============================================================================//

namespace sts::actions {

//----------------------------------------------------------------------------//

struct Sara_Base : public BaseAction<Sara_Fighter>
{
    using BaseAction<Sara_Fighter>::BaseAction;
};

//----------------------------------------------------------------------------//

struct Sara_Neutral_First final : public Sara_Base
{
    void on_start() override
    {
        Sara_Fighter& fighter = get_fighter();
        fighter.play_animation(fighter.ANIM_Action_Neutral_First);
    }

    bool on_tick(uint frame) override
    {
        if (frame == 2u) world.enable_hit_blob(blobs["0a"]);
        if (frame == 4u) world.enable_hit_blob(blobs["0b"]);

        if (frame == 6u) world.disable_hit_blob(blobs["0a"]);
        if (frame == 8u) world.disable_hit_blob(blobs["0b"]);

        return frame >= 14u;
    }

    void on_collide(HitBlob* blob, Fighter& other) override
    {
        other.apply_hit_basic(*blob);
    }

    void on_finish() override
    {
        //remove_hit_blobs(blobs[0], blobs[1]);
        reset_hit_blob_groups(0u);
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Down final : public Sara_Base
{
    void on_start() override
    {
        Sara_Fighter& fighter = get_fighter();
        fighter.play_animation(fighter.ANIM_Action_Tilt_Down);
    }

    bool on_tick(uint frame) override
    {
        if (frame ==  4u) world.enable_hit_blob(blobs["0a"]);
        if (frame ==  6u) world.enable_hit_blob(blobs["1a"]);
        if (frame == 10u) world.enable_hit_blob(blobs["2a"]);

        if (frame ==  6u) world.disable_hit_blob(blobs["0a"]);
        if (frame == 10u) world.disable_hit_blob(blobs["1a"]);
        if (frame == 16u) world.disable_hit_blob(blobs["2a"]);

        return frame >= 26u;
    }

    void on_collide(HitBlob* blob, Fighter& other) override
    {
        other.apply_hit_basic(*blob);
    }

    void on_finish() override
    {
        //remove_hit_blobs(blobs[0], blobs[1], blobs[2]);
        reset_hit_blob_groups(0u, 1u, 2u);
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Forward final : public Sara_Base
{
    void on_start() override
    {
        Sara_Fighter& fighter = get_fighter();
        fighter.play_animation(fighter.ANIM_Action_Tilt_Forward);
    }

    bool on_tick(uint frame) override
    {
//        if (frame ==  4u) blobs[0] = add_hit_blob(0u, Flavour::Tangy, Priority::Normal);
//        if (frame ==  7u) blobs[1] = add_hit_blob(1u, Flavour::Sour, Priority::Normal);
//        if (frame ==  7u) blobs[2] = add_hit_blob(1u, Flavour::Sour, Priority::Normal);

//        if (frame ==  7u) remove_hit_blobs(blobs[0]);
//        if (frame == 14u) remove_hit_blobs(blobs[1], blobs[2]);

//        set_blob_sphere_relative(blobs[0], { 0.65f, 1.0f, 0.f }, 0.5f);
//        set_blob_sphere_relative(blobs[1], { 0.8f, 0.8f, 0.f }, 0.55f);
//        set_blob_sphere_relative(blobs[2], { 0.8f, 1.2f, 0.f }, 0.55f);

        return frame >= 22u;
    }

    void on_collide(HitBlob* blob, Fighter& other) override
    {
//        if (blob == blobs[0]) std::cout << "sweet hit!" << std::endl;
//        if (blob == blobs[1]) std::cout << "sour hit!"  << std::endl;
//        if (blob == blobs[2]) std::cout << "sour hit!"  << std::endl;
    }

    void on_finish() override
    {
//        remove_hit_blobs(blobs[0], blobs[1], blobs[2]);
//        reset_hit_blob_groups(0u, 1u);
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Up final : public Sara_Base
{
    void on_start() override
    {
        Sara_Fighter& fighter = get_fighter();
        fighter.play_animation(fighter.ANIM_Action_Tilt_Up);
    }

    bool on_tick(uint frame) override
    {
//        if (frame ==  3u) blobs[0] = add_hit_blob(0u, Flavour::Tangy, Priority::Normal);
//        if (frame ==  4u) blobs[1] = add_hit_blob(1u, Flavour::Sour, Priority::Normal);
//        if (frame ==  6u) blobs[2] = add_hit_blob(1u, Flavour::Sweet, Priority::High);

//        if (frame ==  8u) remove_hit_blobs(blobs[0], blobs[2]);
//        if (frame == 11u) remove_hit_blobs(blobs[1]);

//        set_blob_sphere_relative(blobs[0], { 0.25f, 1.6f, 0.f }, 0.4f);
//        set_blob_sphere_relative(blobs[1], { 0.12f, 2.0f, 0.f }, 0.6f);
//        set_blob_sphere_relative(blobs[2], { 0.12f, 2.5f, 0.f }, 0.2f);

        return frame >= 20u;
    }

    void on_collide(HitBlob* blob, Fighter& other) override
    {
//        if (blob == blobs[0]) std::cout << "tangy hit!" << std::endl;
//        if (blob == blobs[1]) std::cout << "sour hit!"  << std::endl;
//        if (blob == blobs[2]) std::cout << "sweet hit!" << std::endl;
    }

    void on_finish() override
    {
//        remove_hit_blobs(blobs[0], blobs[1], blobs[2]);
//        reset_hit_blob_groups(0u, 1u);
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
    actions->dash_attack   = std::make_unique<DumbAction>(world, fighter, "Sara Dash_Attack");

    actions->load_json("Sara");

    return actions;
}
