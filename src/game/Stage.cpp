#include "game/Stage.hpp"

#include "game/FightWorld.hpp"
#include "game/Fighter.hpp"

#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"
#include "render/Camera.hpp"

#include <sqee/maths/Culling.hpp>
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
    mSkyboxPath = render.at("skybox");

    const JsonValue& general = json.at("general");
    mInnerBoundary.min = general.at("inner_boundary_min");
    mInnerBoundary.max = general.at("inner_boundary_max");
    mOuterBoundary.min = general.at("outer_boundary_min");
    mOuterBoundary.max = general.at("outer_boundary_max");

    for (const JsonValue& jblock : json.at("blocks"))
    {
        AlignedBlock& block = mAlignedBlocks.emplace_back();
        block.minimum = jblock.at("minimum");
        block.maximum = jblock.at("maximum");
    }

    for (const JsonValue& jledge : json.at("ledges"))
    {
        Ledge& ledge = mLedges.emplace_back();
        ledge.position = jledge.at("position");
        ledge.direction = jledge.at("direction");
    }

    for (const JsonValue& jplatform : json.at("platforms"))
    {
        Platform& platform = mPlatforms.emplace_back();
        platform.originY = jplatform.at("originY");
        platform.minX = jplatform.at("minX");
        platform.maxX = jplatform.at("maxX");
    }

    const auto& ctx = sq::VulkanContext::get();

    // load environment maps
    {
        world.renderer.cubemaps.skybox.load_from_file_cube(sq::build_string("assets/", mSkyboxPath + "/Sky"));
        world.renderer.cubemaps.irradiance.load_from_file_cube(sq::build_string("assets/", mSkyboxPath + "/Irradiance"));
        world.renderer.cubemaps.radiance.load_from_file_cube(sq::build_string("assets/", mSkyboxPath + "/Radiance"));

        sq::vk_update_descriptor_set (
            ctx, world.renderer.sets.skybox,
            sq::DescriptorImageSampler(0u, 0u, world.renderer.cubemaps.skybox.get_descriptor_info())
        );

        sq::vk_update_descriptor_set_swapper (
            ctx, world.renderer.sets.environment,
            sq::DescriptorImageSampler(1u, 0u, world.renderer.cubemaps.irradiance.get_descriptor_info()),
            sq::DescriptorImageSampler(2u, 0u, world.renderer.cubemaps.radiance.get_descriptor_info())
        );
    }

    // uniform buffer and descriptor set
    {
        mStaticUbo.initialise(sizeof(StaticBlock), vk::BufferUsageFlagBits::eUniformBuffer);
        mDescriptorSet = sq::vk_allocate_descriptor_set_swapper(ctx, world.renderer.setLayouts.object);

        sq::vk_update_descriptor_set_swapper (
            ctx, mDescriptorSet, sq::DescriptorUniformBuffer(0u, 0u, mStaticUbo.get_descriptor_info())
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
}

//============================================================================//

MoveAttempt Stage::attempt_move(const LocalDiamond& diamond, Vec2F current, Vec2F target, bool edgeStop)
{
//    const WorldDiamond targetDiamond = diamond.translate(translation);
//    const Vec2F target = targetDiamond.origin();

    //--------------------------------------------------------//

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
            if (current.y >= block.maximum.y && target.y < block.maximum.y)
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

    for (const auto& platform : mPlatforms)
    {
        const float currentMinY = current.y + diamond.min().y;
        const Vec2F currentCross = current + diamond.cross();

        const float targetMinY = target.y + diamond.min().y;
        const Vec2F targetCross = target + diamond.cross();

        if (currentMinY >= platform.originY && targetMinY < platform.originY)
        {
            if (edgeStop == true)
            {
                if (currentCross.x >= platform.minX && targetCross.x <= platform.minX)
                {
                    attempt.result = { platform.minX, platform.originY };
                    attempt.type = MoveAttempt::Type::EdgeStop;
                    attempt.edge = -1;
                    attempt.collideFloor = true; // fighter allowed to ignore this
                }
                if (currentCross.x <= platform.maxX && targetCross.x >= platform.maxX)
                {
                    attempt.result = { platform.maxX, platform.originY };
                    attempt.type = MoveAttempt::Type::EdgeStop;
                    attempt.edge = +1;
                    attempt.collideFloor = true; // fighter allowed to ignore this
                }
            }

            if (targetCross.x >= platform.minX && targetCross.x <= platform.maxX)
            {
                // todo: convey that this is a platform that can be dropped through
                attempt.result.y = maths::max(attempt.result.y, target.y, platform.originY);
                attempt.collideFloor = true;
            }
        }
    }

    //--------------------------------------------------------//

    return attempt;
}

//============================================================================//

Ledge* Stage::find_ledge(Vec2F position, int8_t direction)
{
    constexpr const float GRAB_DISTANCE = 1.2f;

    for (auto& ledge : mLedges)
    {
        if (ledge.direction == direction)
            continue;
        if (position.y >= ledge.position.y)
            continue;

        if (maths::distance_squared(position, ledge.position) > GRAB_DISTANCE * GRAB_DISTANCE)
            continue;

        if (direction > 0)
            if (position.x < ledge.position.x)
                return &ledge;

        if (direction < 0)
            if (position.x > ledge.position.x)
                return &ledge;
    }

    return nullptr;
}

//============================================================================//

void Stage::check_boundary(Fighter& fighter)
{
    const Vec2F centre = fighter.status.position + fighter.get_diamond().cross();

    if ( centre.x < mOuterBoundary.min.x || centre.x > mOuterBoundary.max.x ||
         centre.y < mOuterBoundary.min.y || centre.y > mOuterBoundary.max.y )
    {
        fighter.pass_boundary();
    }
}
