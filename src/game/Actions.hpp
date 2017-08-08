#pragma once

#include <sqee/builtins.hpp>
#include <sqee/misc/PoolTools.hpp>

#include "game/HitBlob.hpp"
#include "game/FightSystem.hpp"

#include "game/forward.hpp"

namespace sts {

//============================================================================//

class Action : public sq::NonCopyable
{
public: //====================================================//

    enum class Type
    {
        None = -1,
        Neutral_First,
        Tilt_Down,
        Tilt_Forward,
        Tilt_Up,
        Air_Back,
        Air_Down,
        Air_Forward,
        Air_Neutral,
        Air_Up,
        Dash_Attack,
    };

    //--------------------------------------------------------//

    Action(FightWorld& world, Fighter& fighter);

    virtual ~Action();

    //--------------------------------------------------------//

    /// Do stuff when action first starts.
    virtual void on_start() {}

    /// Return true when the action is finished.
    virtual bool on_tick(uint frame) = 0;

    /// Do stuff when finished or cancelled.
    virtual void on_finish() = 0;

    /// Do stuff when a blob collides with a fighter.
    virtual void on_collide(HitBlob* blob, Fighter& other) = 0;

protected: //=================================================//

    /// Jump action to the specified frame.
    void jump_to_frame(uint frame);

    //--------------------------------------------------------//

    FightWorld& world;
    Fighter& fighter;

    sq::TinyPoolMap<HitBlob> blobs;

private: //===================================================//

    uint mCurrentFrame = 0u;

    char _padding[4];

    //--------------------------------------------------------//

    friend class Actions;
};

//============================================================================//

class Actions final : public sq::NonCopyable
{
public: //====================================================//

    /// Load JSON data for all actions.
    void load_json(const string& fighterName);

    //--------------------------------------------------------//

    void switch_active(Action::Type type)
    {
        if (mActiveAction != nullptr) mActiveAction->on_finish();

        if (type == Action::Type::None)          mActiveAction = nullptr;
        if (type == Action::Type::Neutral_First) mActiveAction = neutral_first.get();
        if (type == Action::Type::Tilt_Down)     mActiveAction = tilt_down.get();
        if (type == Action::Type::Tilt_Forward)  mActiveAction = tilt_forward.get();
        if (type == Action::Type::Tilt_Up)       mActiveAction = tilt_up.get();
        if (type == Action::Type::Air_Back)      mActiveAction = air_back.get();
        if (type == Action::Type::Air_Down)      mActiveAction = air_down.get();
        if (type == Action::Type::Air_Forward)   mActiveAction = air_forward.get();
        if (type == Action::Type::Air_Neutral)   mActiveAction = air_neutral.get();
        if (type == Action::Type::Air_Up)        mActiveAction = air_up.get();
        if (type == Action::Type::Dash_Attack)   mActiveAction = dash_attack.get();

        mActiveType = type;

        mActiveAction->mCurrentFrame = 0u;
        mActiveAction->on_start();
    }

    //--------------------------------------------------------//

    /// Get the type of the active action.
    auto active_type() const { return mActiveType; }

    /// Get the active action.
    auto active_action() const { return mActiveAction; }

    //--------------------------------------------------------//

    /// Tick the active action if not none.
    void tick_active_action();

    /// Update blobs of the active action if not none.
    void update_active_action_blobs();

    //--------------------------------------------------------//

    unique_ptr<Action> neutral_first;

    unique_ptr<Action> tilt_down;
    unique_ptr<Action> tilt_forward;
    unique_ptr<Action> tilt_up;

    unique_ptr<Action> air_back;
    unique_ptr<Action> air_down;
    unique_ptr<Action> air_forward;
    unique_ptr<Action> air_neutral;
    unique_ptr<Action> air_up;

    unique_ptr<Action> dash_attack;

private: //===================================================//

    Action::Type mActiveType = Action::Type::None;
    Action* mActiveAction = nullptr;
};

//============================================================================//

class DumbAction final : public Action
{
public: //====================================================//

    DumbAction(FightWorld& world, Fighter& fighter, string message)
        : Action(world, fighter), message(message) {}

private: //===================================================//

    void on_start() override;

    bool on_tick(uint frame) override;

    void on_collide(HitBlob*, Fighter&) override;

    void on_finish() override;

    //--------------------------------------------------------//

    const string message;
};

//============================================================================//

/// Helper base class for defining new actions.
///
/// @tparam ConcreteFighter Type of this action's fighter.

template <class ConcreteFighter>
struct BaseAction : public Action
{
    //--------------------------------------------------------//

    using Flavour = HitBlob::Flavour;
    using Priority = HitBlob::Priority;

    //--------------------------------------------------------//

    BaseAction(FightWorld& world, ConcreteFighter& fighter) : Action(world, fighter) {}

    ConcreteFighter& get_fighter() { return static_cast<ConcreteFighter&>(fighter); }

    //--------------------------------------------------------//

    template <class... Args>
    void reset_hit_blob_groups(const Args... groups)
    {
        static_assert(( std::is_convertible_v<Args, uint8_t> && ... ));
        ( world.reset_hit_blob_group(fighter, uint8_t(groups)) , ... );
    }

    //--------------------------------------------------------//
};

//============================================================================//

} // namespace sts
