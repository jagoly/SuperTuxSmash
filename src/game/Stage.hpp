#pragma once

#include <sqee/builtins.hpp>

//============================================================================//

namespace sts {

class Stage : sq::NonCopyable
{
public: //====================================================//

    Stage();

    virtual ~Stage() = default;

    //--------------------------------------------------------//

    virtual void tick() = 0;

protected: //=================================================//

};

} // namespace sts
