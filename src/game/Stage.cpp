#include "game/Stage.hpp"

#include "game/Fighter.hpp"

#include <sqee/maths/Culling.hpp>

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Stage::Stage(FightWorld& world) : mFightWorld(world) {}

//============================================================================//

MoveAttempt Stage::attempt_move(WorldDiamond diamond, Vec2F translation)
{
    const WorldDiamond targetDiamond = diamond.translated(translation);
    const Vec2F target = targetDiamond.origin();

    //--------------------------------------------------------//

    MoveAttempt attempt;
    attempt.result = target;

    Vec2F& result = attempt.result;

    //--------------------------------------------------------//

    for (const auto& block : mAlignedBlocks)
    {
        const bool originNegX = diamond.negX >= block.maximum.x; // left is right of right
        const bool originNegY = diamond.negY >= block.maximum.y; // bottom is above top
        const bool originPosX = diamond.posX <= block.minimum.x; // right is left of left
        const bool originPosY = diamond.posY <= block.minimum.y; // top is below bottom

        const bool targetNegX = targetDiamond.negX >= block.maximum.x; // left is right of right
        const bool targetNegY = targetDiamond.negY >= block.maximum.y; // bottom is above top
        const bool targetPosX = targetDiamond.posX <= block.minimum.x; // right is left of left
        const bool targetPosY = targetDiamond.posY <= block.minimum.y; // top is below bottom

        const bool crossInX = targetDiamond.crossX >= block.minimum.x && targetDiamond.crossX <= block.maximum.x;
        const bool crossInY = targetDiamond.crossY >= block.minimum.y && targetDiamond.crossY <= block.maximum.y;

        SQASSERT(!(originNegX && originPosX), "");
        SQASSERT(!(originNegY && originPosY), "");
        SQASSERT(!(targetNegX && targetPosX), "");
        SQASSERT(!(targetNegY && targetPosY), "");

        if (crossInX && originNegY && !targetNegY) // test bottom into floor
        {
            result.y = maths::max(result.y, block.maximum.y);
            attempt.floor = MoveAttempt::Floor::Solid;
        }

        if (crossInX && originPosY && !targetPosY) // test top into ceiling
        {
            result.y = maths::min(result.y, block.minimum.y + (diamond.negY - diamond.posY));
        }

        if (crossInY && originNegX && !targetNegX) // test left into wall
        {
            result.x = maths::max(result.x, block.maximum.x + (diamond.crossX - diamond.negX));
        }

        if (crossInY && originPosX && !targetPosX) // test right into wall
        {
            result.x = maths::min(result.x, block.minimum.x + (diamond.crossX - diamond.posX));
        }
    }

    //--------------------------------------------------------//

    for (const auto& platform : mPlatforms)
    {
        const bool originNegY = diamond.negY >= platform.originY;
        const bool targetNegY = targetDiamond.negY >= platform.originY;

        if (diamond.negY == platform.originY)
        {
            if (diamond.crossX >= platform.minX && targetDiamond.crossX <= platform.minX)
            {
                result = { platform.minX, platform.originY };
                attempt.type = MoveAttempt::Type::EdgeStop;
                attempt.edge = -1;
            }

            if (diamond.crossX <= platform.maxX && targetDiamond.crossX >= platform.maxX)
            {
                result = { platform.maxX, platform.originY };
                attempt.type = MoveAttempt::Type::EdgeStop;
                attempt.edge = +1;
            }
        }

        if (originNegY && !targetNegY)
        {
            if (targetDiamond.crossX >= platform.minX && targetDiamond.crossX <= platform.maxX)
            {
                result.y = maths::max(result.y, target.y, platform.originY);
                attempt.floor = MoveAttempt::Floor::Platform;
            }
        }
    }

    //--------------------------------------------------------//

    return attempt;
}

//============================================================================//

void Stage::check_boundary(Fighter& fighter)
{
    const Vec2F position = fighter.get_diamond().centre();

    if ( position.x < mOuterBoundary.min.x || position.x > mOuterBoundary.max.x ||
         position.y < mOuterBoundary.min.y || position.y > mOuterBoundary.max.y )
    {
        fighter.pass_boundary();
    }
}
