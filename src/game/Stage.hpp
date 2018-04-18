#pragma once

#include <sqee/builtins.hpp>

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

    Type type = Type::Simple;
    Vec2F result;

    enum class Floor { None, Platform, Solid, Slope };

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

    std::vector<Platform> mPlatforms;

    std::vector<AlignedBlock> mAlignedBlocks;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;
};

//============================================================================//

#define ETSC SQEE_ENUM_TO_STRING_CASE

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(MoveAttempt::Type)
ETSC(Simple) ETSC(Slope) ETSC(EdgeStop)
SQEE_ENUM_TO_STRING_BLOCK_END

SQEE_ENUM_TO_STRING_BLOCK_BEGIN(MoveAttempt::Floor)
ETSC(None) ETSC(Platform) ETSC(Solid) ETSC(Slope)
SQEE_ENUM_TO_STRING_BLOCK_END

#undef ETSC

//============================================================================//

} // namespace sts
