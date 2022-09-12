#pragma once

#include "setup.hpp"

#include "render/AnimPlayer.hpp"
#include "render/Environment.hpp"

#include <sqee/objects/Armature.hpp>
#include <sqee/objects/DrawItem.hpp>

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

    Stage(World& world, TinyString name);

    ~Stage();

    void tick();

    void integrate(float blend);

    //--------------------------------------------------------//

    World& world;

    /// Short name of this stage.
    const TinyString name;

    //--------------------------------------------------------//

    MoveAttempt attempt_move(const LocalDiamond& diamond, Vec2F current, Vec2F target, bool edgeStop, bool ignorePlatforms);

    MoveAttemptSphere attempt_move_sphere(float radius, float bounceFactor, Vec2F position, Vec2F velocity, bool ignorePlatforms);

    Ledge* find_ledge(const LocalDiamond& diamond, Vec2F origin, int8_t facing, int8_t inputX);

    bool check_point_out_of_bounds(Vec2F point);

    //--------------------------------------------------------//

    const Environment& get_environment() const { return mEnvironment; }

    const String& get_skybox_path() const { return mSkyboxPath; }

    const MinMax<Vec2F>& get_inner_boundary() const { return mInnerBoundary; }
    const MinMax<Vec2F>& get_outer_boundary() const { return mOuterBoundary; }
    const MinMax<Vec3F>& get_shadow_casters() const { return mShadowCasters; }

protected: //=================================================//

    Environment mEnvironment;

    sq::Armature mArmature;

    std::vector<sq::DrawItem> mDrawItems;

    AnimPlayer mAnimPlayer;

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

    friend EditorScene;
    friend DebugGui;
};

//============================================================================//

} // namespace sts

WRENPLUS_TRAITS_HEADER(sts::Ledge)
WRENPLUS_TRAITS_HEADER(sts::Stage)
