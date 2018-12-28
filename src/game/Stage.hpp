#pragma once

#include <sqee/misc/Builtins.hpp>
#include <sqee/maths/Builtins.hpp>

#include "game/FightWorld.hpp"

namespace sts {

//============================================================================//

struct Platform
{
    float originY;
    float minX, maxX;
};

struct AlignedBlock
{
    Vec2F minimum;
    Vec2F maximum;
};

//============================================================================//

struct MoveAttempt
{
    enum class Type { Simple, Slope, EdgeStop };
    enum class Floor { None, Platform, Solid, Slope };

    Type type = Type::Simple;
    Vec2F result;

    Floor floor = Floor::None;

    // non-zero if platform edge reached
    int8_t edge = 0;
};

//============================================================================//

class Stage : sq::NonCopyable
{
public: //====================================================//

    Stage(FightWorld& world);

    virtual ~Stage() = default;

    virtual void tick() = 0;

    //--------------------------------------------------------//

    MoveAttempt attempt_move(WorldDiamond diamond, Vec2F translation);

    void check_boundary(Fighter& fighter);

    //--------------------------------------------------------//

    const auto& get_inner_boundary() const { return mInnerBoundary; }
    const auto& get_outer_boundary() const { return mOuterBoundary; }

protected: //=================================================//

    struct { Vec2F min, max; } mInnerBoundary;
    struct { Vec2F min, max; } mOuterBoundary;

    //--------------------------------------------------------//

    Vector<Platform> mPlatforms;

    Vector<AlignedBlock> mAlignedBlocks;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::MoveAttempt::Type, Simple, Slope, EdgeStop)

SQEE_ENUM_HELPER(sts::MoveAttempt::Floor, None, Platform, Solid, Slope)
