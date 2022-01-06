#pragma once

#include "setup.hpp"

#include "main/MainEnums.hpp"

#include <sqee/vk/SwapBuffer.hpp>

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

struct Ledge
{
    /// The point that fighters will grab on to.
    Vec2F position;

    /// Left = -1, Right = +1 (will be opposite of grabber facing)
    int8_t direction;

    /// The fighter currently hanging from the ledge.
    Fighter* grabber = nullptr;
};

//============================================================================//

class Stage final : sq::NonCopyable
{
public: //====================================================//

    Stage(World& world, StageEnum type);

    ~Stage();

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    World& world;

    const StageEnum type;

    //--------------------------------------------------------//

    MoveAttempt attempt_move(const LocalDiamond& diamond, Vec2F current, Vec2F target, bool edgeStop, bool ignorePlatforms);

    Ledge* find_ledge(const LocalDiamond& diamond, Vec2F origin, int8_t facing, int8_t inputX);

    bool check_point_out_of_bounds(Vec2F point);

    //--------------------------------------------------------//

    const String& get_skybox_path() const { return mSkyboxPath; }

    const MinMax<Vec2F>& get_inner_boundary() const { return mInnerBoundary; }
    const MinMax<Vec2F>& get_outer_boundary() const { return mOuterBoundary; }
    const MinMax<Vec3F>& get_shadow_casters() const { return mShadowCasters; }

protected: //=================================================//

    MinMax<Vec2F> mInnerBoundary;
    MinMax<Vec2F> mOuterBoundary;
    MinMax<Vec3F> mShadowCasters;

    //--------------------------------------------------------//

    String mSkyboxPath;

    Vec3F mLightDirection;
    Vec3F mLightColour;

    //--------------------------------------------------------//

    std::vector<Platform> mPlatforms;

    std::vector<AlignedBlock> mAlignedBlocks;

    std::vector<Ledge> mLedges;

    //--------------------------------------------------------//

    sq::SwapBuffer mStaticUbo;
    sq::Swapper<vk::DescriptorSet> mDescriptorSet;

    friend EditorScene;
    friend DebugGui;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::Ledge)
WRENPLUS_TRAITS_HEADER(sts::Stage)
