#pragma once

#include <sqee/builtins.hpp>

namespace sts {

//============================================================================//

class Fighter; // Forward Declaration

//============================================================================//

enum class AttackType
{
    Neutral_First  =  0u,
    Neutral_Second =  1u,
    Neutral_Third  =  2u,

    Tilt_Forward   =  3u,
    Tilt_Down      =  4u,
    Tilt_Up        =  5u,

    Air_Neutral    =  6u,
    Air_Back       =  7u,
    Air_Forward    =  8u,
    Air_Down       =  9u,
    Air_Up         = 10u,

    Dashing        = 11u,
};

//============================================================================//

class Attack : sq::NonCopyable
{
public:

    //========================================================//

    Attack(AttackType type);

    virtual ~Attack();

    //========================================================//

    virtual void on_start();

    virtual bool on_tick();

    virtual void on_finish();

    //========================================================//

    const AttackType type;

    //========================================================//

    struct {

        string message = "";
        uint time = 0u;

    } props;

protected:

    //========================================================//

    uint mTimeLeft = 0u;
};

//============================================================================//

class Attacks : sq::NonCopyable
{
public:

    //========================================================//

    Attacks(string name, Fighter& fighter);

    void make_default_attacks();

    void load_from_json();

    //========================================================//

    unique_ptr<Attack> neutral_first;
    unique_ptr<Attack> neutral_second;
    unique_ptr<Attack> neutral_third;

    unique_ptr<Attack> tilt_down;
    unique_ptr<Attack> tilt_forward;
    unique_ptr<Attack> tilt_up;

//    unique_ptr<Attack> air_neutral;
//    unique_ptr<Attack> air_back;
//    unique_ptr<Attack> air_forward;
//    unique_ptr<Attack> air_down;
//    unique_ptr<Attack> air_up;

//    unique_ptr<Attack> dashing;

    Attack* active = nullptr;

protected:

    const string mName;
    Fighter& mFighter;
};

} // namespace sts
