#pragma once

#include "setup.hpp" // IWYU pragma: export

//============================================================================//

namespace sts {

struct LocalDiamond;

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

struct Ledge
{
    Vec2F position;
    int8_t direction;

    Fighter* grabber = nullptr;
};

//============================================================================//

struct MoveAttempt
{
    enum class Type { Simple, Slope, EdgeStop };

    // todo: I made a FlagSet class at some point, why not use it?
    bool collideFloor = false;
    bool collideWall = false;
    bool collideCeiling = false;
    bool collideCorner = false;

    Type type = Type::Simple;
    Vec2F result;

    // non-zero if platform edge reached
    int8_t edge = 0;
};

//============================================================================//

class Stage : sq::NonCopyable
{
public: //====================================================//

    Stage(FightWorld& world);

    virtual ~Stage();

    virtual void tick() = 0;

    //--------------------------------------------------------//

    MoveAttempt attempt_move(const LocalDiamond& diamond, Vec2F current, Vec2F target, bool edgeStop);

    Ledge* find_ledge(Vec2F position, int8_t direction);

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

    std::vector<Ledge> mLedges;

    //--------------------------------------------------------//

    FightWorld& mFightWorld;
};

//============================================================================//

} // namespace sts

//============================================================================//

SQEE_ENUM_HELPER(sts::MoveAttempt::Type, Simple, Slope, EdgeStop)
