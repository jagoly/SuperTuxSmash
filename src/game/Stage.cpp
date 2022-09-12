#include "game/Stage.hpp"

#include "game/Physics.hpp"
#include "game/World.hpp"

#include "render/Camera.hpp"
#include "render/Renderer.hpp"
#include "render/UniformBlocks.hpp"

#include <sqee/maths/Functions.hpp>
#include <sqee/misc/Json.hpp>

using namespace sts;

//============================================================================//

Stage::Stage(World& world, TinyString name)
    : world(world), name(name)
    , mArmature(fmt::format("assets/stages/{}/Armature.json", name))
    , mAnimPlayer(mArmature)
{
    const JsonValue json = sq::parse_json_from_file(fmt::format("assets/stages/{}/Stage.json", name));

    const JsonValue& render = json.at("render");
    render.at("skybox").get_to(mSkyboxPath);
    render.at("light.direction").get_to(mLightDirection);
    render.at("light.colour").get_to(mLightColour);
    render.at("tonemap.exposure").get_to(mEnvironment.tonemap.exposure);
    render.at("tonemap.contrast").get_to(mEnvironment.tonemap.contrast);
    render.at("tonemap.black").get_to(mEnvironment.tonemap.black);

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

    // load environment maps
    {
        mEnvironment.cubemaps.skybox = world.caches.cubeTextures.acquire(mSkyboxPath + "/Sky");
        mEnvironment.cubemaps.irradiance = world.caches.cubeTextures.acquire(mSkyboxPath + "/Irradiance");
        mEnvironment.cubemaps.radiance = world.caches.cubeTextures.acquire(mSkyboxPath + "/Radiance");

        world.renderer.set_environment(mEnvironment);
        world.renderer.update_cubemap_descriptor_sets();
    }

    mDrawItems = sq::DrawItem::load_from_json (
        fmt::format("assets/stages/{}/Render.json", name), mArmature,
        world.caches.meshes, world.caches.pipelines, world.caches.textures
    );
}

Stage::~Stage() = default;

//============================================================================//

void Stage::tick()
{
}

//============================================================================//

void Stage::integrate(float /*blend*/)
{
    Mat34F* modelMats = world.renderer.reserve_matrices(1u, mAnimPlayer.modelMatsIndex);
    modelMats[0] = Mat34F();

    Mat34F* normalMats = world.renderer.reserve_matrices(1u, mAnimPlayer.normalMatsIndex);
    normalMats[0] = Mat34F();

    const auto check_condition = [&](const TinyString& condition)
    {
        if (condition.empty()) return true;
        SQASSERT(false, "invalid condition");
    };

    for (const sq::DrawItem& item : mDrawItems)
        if (check_condition(item.condition) == true)
            world.renderer.add_draw_call(item, mAnimPlayer);

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

MoveAttemptSphere Stage::attempt_move_sphere(float radius, float bounceFactor, Vec2F position, Vec2F velocity, bool ignorePlatforms)
{
    // This function was designed specifically for Mario's fireball.
    // It has some paramaters, but they may or may not work.

    const Vec2F target = position + velocity;

    MoveAttemptSphere attempt;

    attempt.newPosition = target;
    attempt.newVelocity = velocity;
    attempt.bounced = false;

    const float length = maths::length(velocity);

    // todo: may need to be smaller for moving blocks/platforms
    const float numSteps = std::ceil(length / radius);
    const float stepSize = 1.f / numSteps;

    float responseStep = numSteps + 1.f;
    float responsePenetration = 0.f;
    Vec2F responseOrigin = Vec2F();
    Vec2F responseNormal = Vec2F();

    //--------------------------------------------------------//

    for (const auto& block : mAlignedBlocks)
    {
        // seperate axis test if completely outside of the block
        if (position.x + radius < block.minimum.x && target.x + radius < block.minimum.x) continue;
        if (position.x - radius > block.maximum.x && target.x - radius > block.maximum.x) continue;
        if (position.y + radius < block.minimum.y && target.y + radius < block.minimum.y) continue;
        if (position.y - radius > block.maximum.y && target.y - radius > block.maximum.y) continue;

        // only check steps earlier than the earliest collision already found
        for (float step = 0.f; step < responseStep; step += 1.f)
        {
            const Vec2F stepOrigin = position + velocity * stepSize * step;

            // closest point on or inside the box to the circle
            const Vec2F closest = maths::clamp(stepOrigin, block.minimum, block.maximum);

            const Vec2F difference = stepOrigin - closest;
            const float distSquared = maths::length_squared(difference);

            if (distSquared < radius * radius) // intersects
            {
                responseStep = step;
                responseOrigin = stepOrigin;

                if (distSquared > 0.00001f) // origin is outside of the box
                {
                    const float distance = std::sqrt(distSquared);

                    responsePenetration = radius - distance;
                    responseNormal = difference * (1.f / distance);
                }
                else // origin is inside of the box
                {
                    const float overlapNegX = std::min(block.minimum.x - stepOrigin.x - radius, 0.f);
                    const float overlapPosX = std::max(block.maximum.x - stepOrigin.x + radius, 0.f);

                    const float overlapNegY = std::min(block.minimum.y - stepOrigin.y - radius, 0.f);
                    const float overlapPosY = std::max(block.maximum.y - stepOrigin.y + radius, 0.f);

                    if (true)                               { responsePenetration = +overlapPosY; responseNormal = { 0.f, +1.f }; }
                    if (-overlapNegY < responsePenetration) { responsePenetration = -overlapNegY; responseNormal = { 0.f, -1.f }; }
                    if (+overlapPosX < responsePenetration) { responsePenetration = +overlapPosX; responseNormal = { +1.f, 0.f }; }
                    if (-overlapNegX < responsePenetration) { responsePenetration = -overlapNegX; responseNormal = { -1.f, 0.f }; }
                }

                //sq::log_debug_multiline (
                //    "collision with AlignedBlock\n"
                //    "stepOrigin: {}\nclosest: {}\ndifference: {}\nstep: {}\npenetration: {}\norigin: {}\nnormal: {}",
                //    stepOrigin, closest, difference, responseStep, responsePenetration, responseOrigin, responseNormal
                //);
            }
        }
    }

    //--------------------------------------------------------//

    if (ignorePlatforms == false && velocity.y < 0.f)
    {
        for (const auto& platform : mPlatforms)
        {
            // check if position and target are both to one side of the platform
            if (position.x < platform.minX && target.x < platform.minX) continue;
            if (position.x > platform.maxX && target.x > platform.maxX) continue;

            // check if position is below or intersecting the platform
            if (position.y - radius < platform.originY) continue;

            // check if target is above and not intersecting the platform
            if (target.y - radius > platform.originY) continue;

            // only check steps earlier than the earliest collision already found
            for (float step = 0.f; step < responseStep; step += 1.f)
            {
                const Vec2F stepOrigin = position + velocity * stepSize * step;

                // we ignore intersections with the ends of platform (only bounce on the flat part)
                if (stepOrigin.y - radius > platform.originY) continue;
                if (stepOrigin.x < platform.minX || stepOrigin.x > platform.maxX) continue;

                responseStep = step;
                responseOrigin = stepOrigin;

                responsePenetration = radius - stepOrigin.y + platform.originY;
                responseNormal = Vec2F(0.f, 1.f);

                //sq::log_debug_multiline (
                //    "collision with Platform\n"
                //    "stepOrigin: {}\nstep: {}\npenetration: {}\norigin: {}\nnormal: {}",
                //    stepOrigin, responseStep, responsePenetration, responseOrigin, responseNormal
                //);
            }
        }
    }

    //--------------------------------------------------------//

    if (responseStep <= numSteps)
    {
        attempt.newPosition = responseOrigin + responseNormal * (responsePenetration + 0.00001f);

        const Vec2F incident = velocity * (1.f / length);

        const float nDotI = maths::dot(responseNormal, incident);
        if (nDotI < 0.f)
        {
            const Vec2F reflected = incident - responseNormal * nDotI * 2.f;
            attempt.newVelocity = reflected * length * bounceFactor;
            attempt.bounced = true;
        }

        attempt.newPosition += attempt.newVelocity * ((numSteps - responseStep) / numSteps);

        //sq::log_debug_multiline (
        //    "collided with something\n"
        //    "oldPosition: {}\noldVelocity: {}\nnewPosition: {}\nincident: {}\nnewVelocity: {}",
        //    position, velocity, attempt.newPosition, incident, attempt.newVelocity
        //);
    }

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
