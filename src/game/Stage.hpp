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

class Stage : sq::NonCopyable
{
public: //====================================================//

    Stage(FightWorld& world);

    virtual ~Stage() = default;

    virtual void tick() = 0;

    //--------------------------------------------------------//

    Vec2F transform_response(Vec2F origin, PhysicsDiamond diamond, Vec2F translation);

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

    FightWorld& mFightWorld;
};

//============================================================================//

} // namespace sts
