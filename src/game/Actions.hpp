#pragma once

#include <sqee/builtins.hpp>

namespace sts {

//============================================================================//

class Action : sq::NonCopyable
{
public:

    //========================================================//

    enum class Type
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

    //========================================================//

    Action(Type type);

    virtual ~Action();

    //========================================================//

    virtual void on_start();

    virtual bool on_tick();

    virtual void on_finish();

    //========================================================//

    const Type type;

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

} // namespace sts
