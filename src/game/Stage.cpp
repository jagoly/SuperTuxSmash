#include <sqee/maths/Culling.hpp>

#include "game/Fighter.hpp"
#include "game/Stage.hpp"

namespace maths = sq::maths;
using namespace sts;

//============================================================================//

Stage::Stage(FightWorld& world) : mFightWorld(world) {}

//============================================================================//

TransformResponse Stage::transform_response(WorldDiamond diamond, Vec2F translation)
{
    const WorldDiamond targetDiamond = diamond.translated(translation);
    const Vec2F target = targetDiamond.origin();

    //--------------------------------------------------------//

    TransformResponse response;
    response.result = target;

    Vec2F& result = response.result;

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
            if (diamond.crossX > platform.minX && targetDiamond.crossX < platform.minX)
            {
                result = { platform.minX + 0.001f, platform.originY };
                response.type = TransformResponse::Type::EdgeStop;
            }

            if (diamond.crossX < platform.maxX && targetDiamond.crossX > platform.maxX)
            {
                result = { platform.maxX - 0.001f, platform.originY };
                response.type = TransformResponse::Type::EdgeStop;
            }
        }

        if (originNegY && !targetNegY)
        {
            if (targetDiamond.crossX >= platform.minX && targetDiamond.crossX <= platform.maxX)
            {
                result.y = maths::max(result.y, target.y, platform.originY);
            }
        }
    }

    //--------------------------------------------------------//

    return response;
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
