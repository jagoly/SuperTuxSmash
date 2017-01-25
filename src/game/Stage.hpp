#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

namespace sts {

//============================================================================//

class Stage : sq::NonCopyable
{
public:

    //========================================================//

    Stage() = default;

    virtual ~Stage() = default;

    //========================================================//

    virtual void tick() = 0;
};

//============================================================================//

} // namespace sts
