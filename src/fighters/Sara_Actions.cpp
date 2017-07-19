#include <iostream>

#include "game/FightSystem.hpp"

#include "fighters/Sara_Actions.hpp"

//============================================================================//

namespace sts::actions {

//----------------------------------------------------------------------------//

struct Sara_Base : public Action
{
    using Flavour = HitBlob::Flavour;
    using Priority = HitBlob::Priority;

    Sara_Base(FightSystem& system, Sara_Fighter& fighter) : Action(system, fighter) {}

    Sara_Fighter& get_fighter() { return static_cast<Sara_Fighter&>(fighter); }

    //--------------------------------------------------------//

    Vec2F get_position() const { return fighter.mCurrentPosition; }

    float sign_direction(float x) const { return x * float(fighter.state.direction); }

    //--------------------------------------------------------//

    void set_blob_sphere_relative(HitBlob* blob, Vec3F origin, float radius)
    {
        if (blob == nullptr) return;

        origin.x = sign_direction(origin.x);
        origin.y = sign_direction(origin.y);

        origin.x += get_position().x;
        origin.z += get_position().y;

        blob->sphere = { origin, radius };
    }

    //--------------------------------------------------------//

    HitBlob* add_hit_blob(uint8_t group, Flavour flavour, Priority priority)
    {
        auto blob = system.create_offensive_hit_blob(fighter, *this, group);

        blob->offensive.flavour = flavour;
        blob->offensive.priority = priority;

        return blob;
    }

    //--------------------------------------------------------//

    template <class... Args>
    void remove_hit_blobs(Args*&... blobs)
    {
        static_assert(( std::is_same_v<HitBlob, Args> && ... ));
        auto func = [this](HitBlob*& blob) { if (blob) system.delete_hit_blob(blob); };
        ( func(blobs) , ... ); ( (blobs = nullptr) , ... );
    }

    //--------------------------------------------------------//

    template <class... Args>
    void reset_hit_blob_groups(const Args... groups)
    {
        static_assert(( std::is_arithmetic_v<Args> && ... ));
        ( system.reset_offensive_blob_group(fighter, uint8_t(groups)) , ... );
    }
};

//----------------------------------------------------------------------------//

struct Sara_Neutral_First final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame ==  4u) blobs[0] = add_hit_blob(0u, Flavour::Sour, Priority::Normal);
        if (frame ==  8u) blobs[1] = add_hit_blob(1u, Flavour::Sweet, Priority::Normal);

        if (frame == 10u) remove_hit_blobs(blobs[0], blobs[1]);

        set_blob_sphere_relative(blobs[0], { 0.55f, 0.f, 0.9f }, 0.6f);
        set_blob_sphere_relative(blobs[1], { 1.05f, 0.f, 1.1f }, 0.15f);

        return frame >= 20u;
    }

    void on_collide(HitBlob* blob, Fighter& other, Vec3F point) override
    {
        if (blob == blobs[0]) std::cout << "sour hit!"  << std::endl;
        if (blob == blobs[1]) std::cout << "sweet hit!" << std::endl;
    }

    void on_finish() override
    {
        remove_hit_blobs(blobs[0], blobs[1]);
        reset_hit_blob_groups(0u, 1u);
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Up final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame ==  3u) blobs[0] = add_hit_blob(0u, Flavour::Tangy, Priority::Normal);
        if (frame ==  4u) blobs[1] = add_hit_blob(1u, Flavour::Sour, Priority::Normal);
        if (frame ==  6u) blobs[2] = add_hit_blob(1u, Flavour::Sweet, Priority::High);

        if (frame ==  8u) remove_hit_blobs(blobs[0], blobs[2]);
        if (frame == 11u) remove_hit_blobs(blobs[1]);

        set_blob_sphere_relative(blobs[0], { 0.25f, 0.f, 1.6f }, 0.4f);
        set_blob_sphere_relative(blobs[1], { 0.12f, 0.f, 2.0f }, 0.6f);
        set_blob_sphere_relative(blobs[2], { 0.12f, 0.f, 2.5f }, 0.2f);

        return frame >= 18u;
    }

    void on_collide(HitBlob* blob, Fighter& other, Vec3F point) override
    {
        if (blob == blobs[0]) std::cout << "tangy hit!" << std::endl;
        if (blob == blobs[1]) std::cout << "sour hit!"  << std::endl;
        if (blob == blobs[2]) std::cout << "sweet hit!" << std::endl;
    }

    void on_finish() override
    {
        remove_hit_blobs(blobs[0], blobs[1], blobs[2]);
        reset_hit_blob_groups(0u, 1u);
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Down final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame ==  4u) blobs[0] = add_hit_blob(0u, Flavour::Sweet, Priority::High);
        if (frame ==  6u) blobs[1] = add_hit_blob(1u, Flavour::Sour, Priority::Normal);
        if (frame == 10u) blobs[2] = add_hit_blob(2u, Flavour::Sour, Priority::Normal);

        if (frame ==  6u) remove_hit_blobs(blobs[0]);
        if (frame == 10u) remove_hit_blobs(blobs[1]);
        if (frame == 16u) remove_hit_blobs(blobs[2]);

        set_blob_sphere_relative(blobs[0], { 1.0f, 0.f, 0.1f }, 0.3f);
        set_blob_sphere_relative(blobs[1], { 1.5f, 0.f, 0.1f }, 0.5f);
        set_blob_sphere_relative(blobs[2], { 2.0f, 0.f, 0.1f }, 0.5f);

        return frame >= 24u;
    }

    void on_collide(HitBlob* blob, Fighter& other, Vec3F point) override
    {
        if (blob == blobs[0]) std::cout << "sweet hit!" << std::endl;
        if (blob == blobs[1]) std::cout << "sour hit!"  << std::endl;
        if (blob == blobs[2]) std::cout << "sour hit!"  << std::endl;
    }

    void on_finish() override
    {
        remove_hit_blobs(blobs[0], blobs[1], blobs[2]);
        reset_hit_blob_groups(0u, 1u, 2u);
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

} // namespace sts::actions

//============================================================================//

unique_ptr<sts::Actions> sts::create_actions(FightSystem& system, Sara_Fighter& fighter)
{
    auto actions = std::make_unique<sts::Actions>();

    actions->neutral_first = std::make_unique < actions::Sara_Neutral_First > (system, fighter);
    actions->tilt_down     = std::make_unique < actions::Sara_Tilt_Down     > (system, fighter);
    actions->tilt_forward  = std::make_unique<DumbAction>(system, fighter, "Sara Tilt_Forward");
    actions->tilt_up       = std::make_unique < actions::Sara_Tilt_Up       > (system, fighter);
    actions->air_back      = std::make_unique<DumbAction>(system, fighter, "Sara Air_Back");
    actions->air_down      = std::make_unique<DumbAction>(system, fighter, "Sara Air_Down");
    actions->air_forward   = std::make_unique<DumbAction>(system, fighter, "Sara Air_Forward");
    actions->air_neutral   = std::make_unique<DumbAction>(system, fighter, "Sara Air_Neutral");
    actions->air_up        = std::make_unique<DumbAction>(system, fighter, "Sara Air_Up");
    actions->dash_attack   = std::make_unique<DumbAction>(system, fighter, "Sara Dash_Attack");

    return actions;
}
