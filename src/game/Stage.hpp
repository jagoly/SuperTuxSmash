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

struct TransformResponse
{
    enum class Type { Simple, Slope, EdgeStop };

    Type type = Type::Simple;
    Vec2F result;

    enum class Floor { None, Platform, Solid, Slope, EdgeStop };

    Floor floor = Floor::None;
};

//============================================================================//

class Stage : sq::NonCopyable
{
public: //====================================================//

    Stage(FightWorld& world);

    virtual ~Stage() = default;

    virtual void tick() = 0;

    //--------------------------------------------------------//

    TransformResponse transform_response(WorldDiamond diamond, Vec2F translation);

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

} // namespace sts
