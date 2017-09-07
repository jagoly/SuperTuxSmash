#pragma once

#include <sqee/misc/PoolTools.hpp>

#include "game/FightWorld.hpp"

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
    virtual void on_start() = 0;

    /// Return true when the action is finished.
    virtual bool on_tick(uint frame) = 0;

    /// Do stuff when finished or cancelled.
    virtual void on_finish() = 0;

    /// Do stuff when a blob collides with a fighter.
    virtual void on_collide(HitBlob* blob, Fighter& other) = 0;

protected: //=================================================//

    FightWorld& world;
    Fighter& fighter;

    sq::TinyPoolMap<HitBlob> blobs;

private: //===================================================//

    uint mCurrentFrame = 0u;

    char _padding[4];

    //--------------------------------------------------------//

    friend class Fighter;
    friend class Actions;
};

//============================================================================//

class Actions final : sq::NonCopyable
{
public: //====================================================//

    /// Load JSON data for all actions.
    void load_json(const string& fighterName);

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
};

//============================================================================//

} // namespace sts
