#include "game/Stage.hpp"

#include "game/FightWorld.hpp"
#include "game/Physics.hpp"

#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"
#include "render/Camera.hpp"

#include <sqee/maths/Culling.hpp>
#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>
#include <sqee/vk/Helpers.hpp>

using namespace sts;

//============================================================================//

Stage::Stage(FightWorld& world, StageEnum type)
    : world(world), type(type)
{
    const String path = sq::build_string("assets/stages/", sq::enum_to_string(type));
    const JsonValue json = sq::parse_json_from_file(path + "/Stage.json");

    const JsonValue& render = json.at("render");
    render.at("skybox").get_to(mSkyboxPath);
    render.at("light.direction").get_to(mLightDirection);
    render.at("light.colour").get_to(mLightColour);
    render.at("tonemap.exposure").get_to(world.renderer.tonemap.exposure);
    render.at("tonemap.contrast").get_to(world.renderer.tonemap.contrast);
    render.at("tonemap.black").get_to(world.renderer.tonemap.black);

    const JsonValue& general = json.at("general");
    general.at("inner_boundary_min").get_to(mInnerBoundary.min);
    general.at("inner_boundary_max").get_to(mInnerBoundary.max);
    general.at("outer_boundary_min").get_to(mOuterBoundary.min);
    general.at("outer_boundary_max").get_to(mOuterBoundary.max);
    general.at("shadow_casters_min").get_to(mShadowCasters.min);
    general.at("shadow_casters_max").get_to(mShadowCasters.max);

    for (const JsonValue& jblock : json.at("blocks"))
    {
        AlignedBlock& block = mAlignedBlocks.emplace_back();
        jblock.at("minimum").get_to(block.minimum);
        jblock.at("maximum").get_to(block.maximum);
    }

    for (const JsonValue& jledge : json.at("ledges"))
    {
        Ledge& ledge = mLedges.emplace_back();
        jledge.at("position").get_to(ledge.position);
        jledge.at("direction").get_to(ledge.direction);
    }

    for (const JsonValue& jplatform : json.at("platforms"))
    {
        Platform& platform = mPlatforms.emplace_back();
        jplatform.at("originY").get_to(platform.originY);
        jplatform.at("minX").get_to(platform.minX);
        jplatform.at("maxX").get_to(platform.maxX);
    }

    const auto& ctx = sq::VulkanContext::get();

    // load environment maps
    {
        world.renderer.cubemaps.skybox.load_from_file_cube(sq::build_string("assets/", mSkyboxPath + "/Sky"));
        world.renderer.cubemaps.irradiance.load_from_file_cube(sq::build_string("assets/", mSkyboxPath + "/Irradiance"));
        world.renderer.cubemaps.radiance.load_from_file_cube(sq::build_string("assets/", mSkyboxPath + "/Radiance"));

        world.renderer.update_cubemap_descriptor_sets();
    }

    // uniform buffer and descriptor set
    {
        mStaticUbo.initialise(sizeof(StaticBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        mDescriptorSet = sq::vk_allocate_descriptor_set_swapper(ctx, world.renderer.setLayouts.object);

        sq::vk_update_descriptor_set_swapper (
            ctx, mDescriptorSet,
            sq::DescriptorUniformBuffer(0u, 0u, mStaticUbo.get_descriptor_info())
        );
    }

    world.renderer.create_draw_items(DrawItemDef::load_from_json(path + "/Render.json", world.caches),
                                     mDescriptorSet, {});
}

Stage::~Stage() = default;

//============================================================================//

void Stage::tick()
{
}

//============================================================================//

void Stage::integrate(float /*blend*/)
{
    mDescriptorSet.swap();
    auto& block = *reinterpret_cast<StaticBlock*>(mStaticUbo.swap_map());

    block.matrix = Mat4F();
    //block.normMat = Mat34F(maths::normal_matrix(camera.viewMat));
    block.normMat = Mat34F();

    auto& environmentBlock = *reinterpret_cast<EnvironmentBlock*>(world.renderer.ubos.environment.swap_map());
    environmentBlock.lightColour = mLightColour;
    environmentBlock.lightDirection = maths::normalize(mLightDirection);

    environmentBlock.viewMatrix = maths::look_at_LH(Vec3F(), environmentBlock.lightDirection, Vec3F(0.f, 0.f, 1.f));

    const MinMax<Vec2F> fighters = world.compute_fighter_bounds();
    const Vec3F minimum = maths::min(mShadowCasters.min, Vec3F(fighters.min, +INFINITY));
    const Vec3F maximum = maths::max(mShadowCasters.max, Vec3F(fighters.max, -INFINITY));

    environmentBlock.projViewMatrix = world.renderer.get_camera().compute_light_matrix(environmentBlock.viewMatrix, minimum, maximum);
}

//============================================================================//

MoveAttempt Stage::attempt_move(const LocalDiamond& diamond, Vec2F current, Vec2F target, bool edgeStop, bool ignorePlatforms)
{
    MoveAttempt attempt;

    attempt.result = target;

    //--------------------------------------------------------//

    for (const auto& block : mAlignedBlocks)
    {
        const auto point_in_block = [&block](Vec2F point) -> auto
        {
            struct Result { int8_t x=0, y=0; } result;
            if (point.x < block.minimum.x) result.x = -1;
            if (point.x > block.maximum.x) result.x = +1;
            if (point.y < block.minimum.y) result.y = -1;
            if (point.y > block.maximum.y) result.y = +1;
            return result;
        };

        const auto currentMin = point_in_block(current + diamond.min());
        const auto currentMax = point_in_block(current + diamond.max());

        const auto targetMin = point_in_block(target + diamond.min());
        const auto targetMax = point_in_block(target + diamond.max());

        const auto targetCross = point_in_block(target + diamond.cross());

        // note: this probably does weird stuff if we collide with more than one thing at a time

        if (edgeStop == true)
        {
            if (current.y >= block.maximum.y && target.y <= block.maximum.y)
            {
                if (current.x >= block.minimum.x && target.x <= block.minimum.x) // check for left edge
                {
                    attempt.result = { block.minimum.x, block.maximum.y };
                    attempt.edge = -1;
                    attempt.collideFloor = true;
                }
                if (current.x <= block.maximum.x && target.x >= block.maximum.x) // check for right edge
                {
                    attempt.result = { block.maximum.x, block.maximum.y };
                    attempt.edge = +1;
                    attempt.collideFloor = true;
                }
            }
        }

        if (targetCross.x == 0)
        {
            if (currentMin.y != -1 && targetMin.y != +1) // test bottom into floor
            {
                attempt.result.y = maths::max(attempt.result.y, block.maximum.y);
                attempt.collideFloor = true;
            }
            if (currentMax.y != +1 && targetMax.y != -1) // test top into ceiling
            {
                attempt.result.y = maths::min(attempt.result.y, block.minimum.y - diamond.offsetTop);
                attempt.collideCeiling = true;
            }
        }

        if (targetCross.y == 0)
        {
            if (currentMin.x != -1 && targetMin.x != +1) // test left into wall
            {
                attempt.result.x = maths::max(attempt.result.x, block.maximum.x + diamond.halfWidth);
                attempt.collideWall = true;
            }
            if (currentMax.x != +1 && targetMax.x != -1) // test right into wall
            {
                attempt.result.x = maths::min(attempt.result.x, block.minimum.x - diamond.halfWidth);
                attempt.collideWall = true;
            }
        }

        // todo: this can probably be cleaned up and optimised

        if (edgeStop == false)
        {
            if (targetCross.x == +1 && targetCross.y == +1 && targetMin.x != +1 && targetMin.y != +1)
            {
                const Vec2F corner = { block.maximum.x, block.maximum.y };
                const Vec2F point = { target.x, target.y };

                const float offsetCorner = maths::dot(diamond.normLeftDown, corner);
                const float offsetPoint = maths::dot(diamond.normLeftDown, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::max(attempt.result.x, attempt.result.x + diamond.normLeftDown.x * difference);
                    attempt.result.y = maths::max(attempt.result.y, attempt.result.y + diamond.normLeftDown.y * difference);
                    attempt.collideCorner = true;
                }
            }

            if (targetCross.x == -1 && targetCross.y == +1 && targetMax.x != -1 && targetMin.y != +1)
            {
                const Vec2F corner = { block.minimum.x, block.maximum.y };
                const Vec2F point = { target.x, target.y };

                const float offsetCorner = maths::dot(diamond.normRightDown, corner);
                const float offsetPoint = maths::dot(diamond.normRightDown, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::min(attempt.result.x, attempt.result.x + diamond.normRightDown.x * difference);
                    attempt.result.y = maths::max(attempt.result.y, attempt.result.y + diamond.normRightDown.y * difference);
                    attempt.collideCorner = true;
                }
            }

            if (targetCross.x == +1 && targetCross.y == -1 && targetMin.x != +1 && targetMax.y != -1)
            {
                const Vec2F corner = { block.maximum.x, block.minimum.y };
                const Vec2F point = { target.x, target.y + diamond.offsetTop };

                const float offsetCorner = maths::dot(diamond.normLeftUp, corner);
                const float offsetPoint = maths::dot(diamond.normLeftUp, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::max(attempt.result.x, attempt.result.x + diamond.normLeftUp.x * difference);
                    attempt.result.y = maths::min(attempt.result.y, attempt.result.y + diamond.normLeftUp.y * difference);
                    attempt.collideCorner = true;
                }
            }

            if (targetCross.x == -1 && targetCross.y == -1 && targetMax.x != -1 && targetMax.y != -1)
            {
                const Vec2F corner = { block.minimum.x, block.minimum.y };
                const Vec2F point = { target.x, target.y + diamond.offsetTop };

                const float offsetCorner = maths::dot(diamond.normRightUp, corner);
                const float offsetPoint = maths::dot(diamond.normRightUp, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::min(attempt.result.x, attempt.result.x + diamond.normRightUp.x * difference);
                    attempt.result.y = maths::min(attempt.result.y, attempt.result.y + diamond.normRightUp.y * difference);
                    attempt.collideCorner = true;
                }
            }
        }
    }

    //--------------------------------------------------------//

    if (ignorePlatforms == false)
    {
        for (const auto& platform : mPlatforms)
        {
            const float currentMinY = current.y + diamond.min().y;
            const Vec2F currentCross = current + diamond.cross();

            const float targetMinY = target.y + diamond.min().y;
            const Vec2F targetCross = target + diamond.cross();

            if (currentMinY >= platform.originY && targetMinY <= platform.originY)
            {
                if (edgeStop == true)
                {
                    if (currentCross.x >= platform.minX && targetCross.x <= platform.minX)
                    {
                        attempt.result = { platform.minX, platform.originY };
                        attempt.edge = -1;
                        attempt.collideFloor = true;
                        attempt.onPlatform = true;
                    }
                    if (currentCross.x <= platform.maxX && targetCross.x >= platform.maxX)
                    {
                        attempt.result = { platform.maxX, platform.originY };
                        attempt.edge = +1;
                        attempt.collideFloor = true;
                        attempt.onPlatform = true;
                    }
                }

                if (targetCross.x >= platform.minX && targetCross.x <= platform.maxX)
                {
                    attempt.result.y = maths::max(attempt.result.y, target.y, platform.originY);
                    attempt.collideFloor = true;
                    attempt.onPlatform = true;
                }
            }
        }
    }

    //--------------------------------------------------------//

    return attempt;
}

//============================================================================//

Ledge* Stage::find_ledge(const LocalDiamond& diamond, Vec2F origin, int8_t facing, int8_t inputX)
{
    // todo: smash uses boxes that vary in size depending on character and action
    constexpr float REACH_FRONT = 0.9f;
    constexpr float REACH_BACK = 0.5f;

    for (Ledge& ledge : mLedges)
    {
        // input is pushed away from the ledge
        if (ledge.direction * inputX > 0)
            continue;

        // fighter is above the ledge
        if (ledge.position.y <= origin.y)
            continue;

        // fighter is on the inside of the ledge
        if (float(ledge.direction) * (origin.x - ledge.position.x) <= 0.f)
            continue;

        // todo: replace with an intersection test
        const Vec2F nearest = origin + Vec2F(float(-ledge.direction) * diamond.halfWidth, diamond.offsetCross);
        const float reach = ledge.direction == facing ? REACH_BACK : REACH_FRONT;

        // fighter can't reach the ledge
        if (maths::distance_squared(nearest, ledge.position) > reach * reach)
            continue;

        return &ledge;
    }

    return nullptr;
}

//============================================================================//

bool Stage::check_point_out_of_bounds(Vec2F point)
{
    return ( point.x < mOuterBoundary.min.x || point.x > mOuterBoundary.max.x ||
             point.y < mOuterBoundary.min.y || point.y > mOuterBoundary.max.y );
}
