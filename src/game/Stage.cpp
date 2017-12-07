#include <sqee/maths/Culling.hpp>

#include "game/Fighter.hpp"
#include "game/Stage.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Stage::Stage(FightWorld& world) : mFightWorld(world) {}

//============================================================================//

Vec2F Stage::transform_response(Vec2F origin, PhysicsDiamond diamond, Vec2F translation)
{
    const PhysicsDiamond targetDiamond
    {
        diamond.xNeg + translation, diamond.xPos + translation,
        diamond.yNeg + translation, diamond.yPos + translation
    };

    const Vec2F target = origin + translation;

    //--------------------------------------------------------//

    Vec2F result = target;

    for (const auto& block : mAlignedBlocks)
    {
        const bool originSideNegX = diamond.xNeg.x >= block.maximum.x;
        const bool originSideNegY = diamond.yNeg.y >= block.maximum.y;
        const bool originSidePosX = diamond.xPos.x <= block.minimum.x;
        const bool originSidePosY = diamond.yPos.y <= block.minimum.y;

        const bool targetSideNegX = targetDiamond.xNeg.x >= block.maximum.x;
        const bool targetSideNegY = targetDiamond.yNeg.y >= block.maximum.y;
        const bool targetSidePosX = targetDiamond.xPos.x <= block.minimum.x;
        const bool targetSidePosY = targetDiamond.yPos.y <= block.minimum.y;

        SQASSERT(!(originSideNegX && originSidePosX), "");
        SQASSERT(!(originSideNegY && originSidePosY), "");
        SQASSERT(!(targetSideNegX && targetSidePosX), "");
        SQASSERT(!(targetSideNegY && targetSidePosY), "");

        if (originSideNegY && !targetSideNegY)
        {
            if (targetDiamond.yNeg.x >= block.minimum.x && targetDiamond.yNeg.x <= block.maximum.x)
            {
                const float offset = origin.y - diamond.yNeg.y;
                result.y = maths::max(result.y, target.y, block.maximum.y + offset);
            }
        }

        if (originSidePosY && !targetSidePosY)
        {
            if (targetDiamond.yPos.x >= block.minimum.x && targetDiamond.yPos.x <= block.maximum.x)
            {
                const float offset = origin.y - diamond.yPos.y;
                result.y = maths::min(result.y, target.y, block.minimum.y + offset);
            }
        }

        if (originSideNegX && !targetSideNegX)
        {
            if (targetDiamond.xNeg.y >= block.minimum.y && targetDiamond.xNeg.y <= block.maximum.y)
            {
                const float offset = origin.x - diamond.xNeg.x;
                result.x = maths::max(result.x, target.x, block.maximum.x + offset);
            }
        }

        if (originSidePosY && !targetSidePosY)
        {
            if (targetDiamond.xPos.y >= block.minimum.y && targetDiamond.xPos.y <= block.maximum.y)
            {
                const float offset = origin.x - diamond.xPos.x;
                result.x = maths::min(result.x, target.x, block.minimum.x + offset);
            }
        }
    }

    for (const auto& platform : mPlatforms)
    {
        const bool originSideNegY = diamond.yNeg.y >= platform.originY;
        const bool targetSideNegY = targetDiamond.yNeg.y >= platform.originY;

        if (originSideNegY && !targetSideNegY)
        {
            if (targetDiamond.yNeg.x >= platform.minX && targetDiamond.yNeg.x <= platform.maxX)
            {
                const float offset = origin.y - diamond.yNeg.y;
                result.y = maths::max(result.y, target.y, platform.originY + offset);
            }
        }
    }

    return result;
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
