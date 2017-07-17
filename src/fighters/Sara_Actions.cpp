#include <iostream>

#include "game/FightSystem.hpp"

#include "fighters/Sara_Actions.hpp"

//============================================================================//

namespace sts::actions {

//----------------------------------------------------------------------------//

struct Sara_Base : public Action
{
    Sara_Base(FightSystem& system, Sara_Fighter& fighter) : Action(system, fighter) {}

    Sara_Fighter& get_fighter() { return static_cast<Sara_Fighter&>(fighter); }

    //--------------------------------------------------------//

    Vec2F get_position() { return fighter.mCurrentPosition; }

    float sign_direction(float x) { return x * float(fighter.state.direction); }

    void set_blob_origin(HitBlob* blob, float x, float z, float y = 0.f)
    {
        x += get_position().x; z += get_position().y;
        blob->sphere.origin = { x, y, z };
    }

    //--------------------------------------------------------//

    HitBlob* add_offensive_hit_blob(HitBlob::Flavour flavour, float radius)
    {
        auto blob = system.create_hit_blob(HitBlob::Type::Offensive, fighter, *this);
        blob->offensive.flavour = flavour; blob->sphere.radius = radius;
        return blob;
    }

    auto add_sour_hit_blob  (float radius) { return add_offensive_hit_blob(HitBlob::Flavour::Sour,  radius); }
    auto add_tangy_hit_blob (float radius) { return add_offensive_hit_blob(HitBlob::Flavour::Tangy, radius); }
    auto add_sweet_hit_blob (float radius) { return add_offensive_hit_blob(HitBlob::Flavour::Sweet, radius); }

    //--------------------------------------------------------//

    template <class... Args>
    void remove_hit_blobs(Args*&... blobs)
    {
        static_assert(( std::is_same_v<HitBlob, Args> && ... ));
        ( system.delete_hit_blob(blobs) , ... );
        ( (blobs = nullptr) , ... );
    }
};

//----------------------------------------------------------------------------//

struct Sara_Neutral_First final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame == 20u) return true;

        if (frame ==  4u) blobs[0] = add_sour_hit_blob(0.6f);
        if (frame ==  8u) blobs[1] = add_sweet_hit_blob(0.15f);
        if (frame == 10u) remove_hit_blobs(blobs[0], blobs[1]);

        if (blobs[0]) set_blob_origin(blobs[0], sign_direction(0.55f), 0.9f);
        if (blobs[1]) set_blob_origin(blobs[1], sign_direction(1.05f), 1.1f);

        return false;
    }

    void on_collide(HitBlob* blob, HitBlob* other) override
    {
        if (blob == blobs[0]) std::cout << "sour hit!"  << std::endl;
        if (blob == blobs[1]) std::cout << "sweet hit!" << std::endl;
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Up final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame == 18u) return true;

        if (frame ==  3u) { blobs[0] = add_tangy_hit_blob(0.4f); }
        if (frame ==  4u) { blobs[1] = add_sour_hit_blob(0.6f); }
        if (frame ==  6u) { blobs[2] = add_sweet_hit_blob(0.2f); }
        if (frame ==  8u) { remove_hit_blobs(blobs[0], blobs[2]); }
        if (frame == 11u) { remove_hit_blobs(blobs[1]); }

        if (blobs[0]) set_blob_origin(blobs[0], sign_direction(0.25f), 1.6f);
        if (blobs[1]) set_blob_origin(blobs[1], sign_direction(0.1f), 2.0f);
        if (blobs[2]) set_blob_origin(blobs[2], sign_direction(0.1f), 2.5f);

        return false;
    }

    void on_collide(HitBlob* blob, HitBlob* other) override
    {
        if (blob == blobs[0]) std::cout << "tangy hit!" << std::endl;
        if (blob == blobs[1]) std::cout << "sour hit!"  << std::endl;
        if (blob == blobs[2]) std::cout << "sweet hit!" << std::endl;
    }

    using Sara_Base::Sara_Base;
};

//----------------------------------------------------------------------------//

struct Sara_Tilt_Down final : public Sara_Base
{
    bool on_tick(uint frame) override
    {
        if (frame == 24u) return true;

        if (frame ==  4u) { blobs[0] = add_sweet_hit_blob(0.3f); }
        if (frame ==  6u) { blobs[1] = add_sour_hit_blob(0.5f); remove_hit_blobs(blobs[0]); }
        if (frame == 10u) { blobs[2] = add_sour_hit_blob(0.5f); remove_hit_blobs(blobs[1]); }
        if (frame == 16u) { remove_hit_blobs(blobs[2]); }

        if (blobs[0]) set_blob_origin(blobs[0], sign_direction(1.0f), 0.1f);
        if (blobs[1]) set_blob_origin(blobs[1], sign_direction(1.5f), 0.1f);
        if (blobs[2]) set_blob_origin(blobs[2], sign_direction(2.0f), 0.1f);

        return false;
    }

    void on_collide(HitBlob* blob, HitBlob* other) override
    {
        if (blob == blobs[0]) std::cout << "sweet hit!" << std::endl;
        if (blob == blobs[1]) std::cout << "sour hit!"  << std::endl;
        if (blob == blobs[2]) std::cout << "sour hit!"  << std::endl;
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
