#pragma once

#include <sqee/builtins.hpp>
#include <sqee/maths/Vectors.hpp>

namespace sts {

//============================================================================//

class Game; // Forward Declaration

//============================================================================//

class Stage : sq::NonCopyable
{
public:

    //========================================================//

    Stage(Game& game);

    virtual ~Stage() = default;

    //========================================================//

    virtual void tick() = 0;

private:

    //========================================================//

    Game& mGame;
};

//============================================================================//

} // namespace sts
