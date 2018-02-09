#include "game/Fighter.hpp"

#include "stages/TestZone_Stage.hpp"

using namespace sts;

//============================================================================//

TestZone_Stage::TestZone_Stage(FightWorld& world) : Stage(world)
{
    mInnerBoundary = { {-14.f, -6.f}, {+14.f, +14.f} };
    mOuterBoundary = { {-18.f, -10.f}, {+18.f, +18.f} };

    //--------------------------------------------------------//

    mAlignedBlocks.push_back({{-8.f, -1.f}, {+8.f, 0.f}});

    mAlignedBlocks.push_back({{-0.5f, 6.5f}, {+0.5f, 7.5f}});

    mAlignedBlocks.push_back({{-5.f, 0.f}, {-4.f, 1.f}});

    mPlatforms.push_back({2.5f, -5.3f, -1.7f});
    mPlatforms.push_back({2.5f, +1.7f, +5.3f});

    mPlatforms.push_back({2.5f, -5.3f, -1.7f});
    mPlatforms.push_back({2.5f, +1.7f, +5.3f});

    mPlatforms.push_back({5.0f, -1.8f, +1.8f});
}

//============================================================================//

void TestZone_Stage::tick()
{
}
