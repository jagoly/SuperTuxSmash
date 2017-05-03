#pragma once

#include <sqee/builtins.hpp>

namespace sts {

//============================================================================//

class Fighter; // Forward Declaration

//============================================================================//

class Actions : sq::NonCopyable
{
public:

    //========================================================//

    enum class Type
    {
        None           =  0,
        Neutral_First  =  1,
        Tilt_Forward   =  2,
        Tilt_Down      =  3,
        Tilt_Up        =  4,
        Air_Neutral    =  5,
        Air_Back       =  6,
        Air_Forward    =  7,
        Air_Down       =  8,
        Air_Up         =  9,
        Dash_Attack    = 10,
    };

    //========================================================//

    virtual ~Actions() = default;

    //========================================================//

    virtual bool fn_neutral_first() = 0;

    virtual bool fn_tilt_down() = 0;
    virtual bool fn_tilt_forward() = 0;
    virtual bool fn_tilt_up() = 0;

    virtual bool fn_air_back() = 0;
    virtual bool fn_air_down() = 0;
    virtual bool fn_air_forward() = 0;
    virtual bool fn_air_neutral() = 0;
    virtual bool fn_air_up() = 0;

    virtual bool fn_dash_attack() = 0;

    //========================================================//

    struct {
        std::function<bool()> func;
        Type type = Type::None;
    } active;

};

//============================================================================//

class DebugActions : public Actions
{
public:

    //========================================================//

    DebugActions(Fighter& fighter);

    //========================================================//

    virtual bool fn_neutral_first() override;

    virtual bool fn_tilt_down() override;
    virtual bool fn_tilt_forward() override;
    virtual bool fn_tilt_up() override;

    virtual bool fn_air_back() override;
    virtual bool fn_air_down() override;
    virtual bool fn_air_forward() override;
    virtual bool fn_air_neutral() override;
    virtual bool fn_air_up() override;

    virtual bool fn_dash_attack() override;

protected:

    //========================================================//

    Fighter& mFighter;
    uint mTimeLeft = 0u;
};

//============================================================================//

} // namespace sts
