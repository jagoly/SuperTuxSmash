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
    const auto document = JsonDocument::parse_file(fmt::format("assets/stages/{}/Stage.json", name));
    const auto json = document.root().as<JsonObject>();

    const auto jRender = json["render"].as<JsonObject>();
    mSkyboxPath = jRender["skybox"].as_auto();
    mLightDirection = jRender["light.direction"].as_auto();
    mLightColour = jRender["light.colour"].as_auto();
    mEnvironment.tonemap.exposure = jRender["tonemap.exposure"].as_auto();
    mEnvironment.tonemap.contrast = jRender["tonemap.contrast"].as_auto();
    mEnvironment.tonemap.black = jRender["tonemap.black"].as_auto();

    const auto jGeneral = json["general"].as<JsonObject>();
    mInnerBoundary.min = jGeneral["inner_boundary_min"].as_auto();
    mInnerBoundary.max = jGeneral["inner_boundary_max"].as_auto();
    mOuterBoundary.min = jGeneral["outer_boundary_min"].as_auto();
    mOuterBoundary.max = jGeneral["outer_boundary_max"].as_auto();
    mShadowCasters.min = jGeneral["shadow_casters_min"].as_auto();
    mShadowCasters.max = jGeneral["shadow_casters_max"].as_auto();

    for (const auto [_, jBlock] : json["blocks"].as<JsonObject>() | views::json_as<JsonObject>)
    {
        AlignedBlock& block = mAlignedBlocks.emplace_back();
        block.minimum = jBlock["minimum"].as_auto();
        block.maximum = jBlock["maximum"].as_auto();
    }

    for (const auto [_, jLedge] : json["ledges"].as<JsonObject>() | views::json_as<JsonObject>)
    {
        Ledge& ledge = mLedges.emplace_back();
        ledge.position = jLedge["position"].as_auto();
        ledge.direction = jLedge["direction"].as_auto();
    }

    for (const auto [_, jPlatform] : json["platforms"].as<JsonObject>() | views::json_as<JsonObject>)
    {
        Platform& platform = mPlatforms.emplace_back();
        platform.originY = jPlatform["originY"].as_auto();
        platform.minX = jPlatform["minX"].as_auto();
        platform.maxX = jPlatform["maxX"].as_auto();
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

    // todo: change to wren expressions
    for (const sq::DrawItem& drawItem : mDrawItems)
    {
        if (drawItem.condition.empty()) continue;
        sq::log_warning("'assets/stage/{}/Render.json': invalid condition '{}'", name, drawItem.condition);
    }
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
        return true; // invalid
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

MoveAttempt Stage::attempt_move(const Diamond& current, const Diamond& target, bool edgeStop, bool ignorePlatforms)
{
    MoveAttempt attempt;

    attempt.result = Vec2F(target.cross.x, target.min.y);

    const Vec2F normLeftDown  = maths::normalize(Vec2F(target.min.y - target.cross.y, target.min.x - target.cross.x));
    const Vec2F normLeftUp    = maths::normalize(Vec2F(target.cross.y - target.max.y, target.cross.x - target.min.x));
    const Vec2F normRightDown = maths::normalize(Vec2F(target.cross.y - target.min.y, target.cross.x - target.max.x));
    const Vec2F normRightUp   = maths::normalize(Vec2F(target.max.y - target.cross.y, target.max.x - target.cross.x));

    //--------------------------------------------------------//

    for (const auto& block : mAlignedBlocks)
    {
        // todo: this is a seperate axis test, "point" is probably misleading
        const auto point_in_block = [&block](Vec2F point) -> auto
        {
            struct Result { int8_t x=0, y=0; } result;
            if (point.x < block.minimum.x) result.x = -1;
            if (point.x > block.maximum.x) result.x = +1;
            if (point.y < block.minimum.y) result.y = -1;
            if (point.y > block.maximum.y) result.y = +1;
            return result;
        };

        const auto testCurrentMin = point_in_block(current.min);
        const auto testCurrentMax = point_in_block(current.max);

        const auto testTargetMin = point_in_block(target.min);
        const auto testTargetMax = point_in_block(target.max);

        const auto testTargetCross = point_in_block(target.cross);

        // note: this probably does weird stuff if we collide with more than one thing at a time

        if (edgeStop == true)
        {
            if (current.min.y >= block.maximum.y && target.min.y <= block.maximum.y)
            {
                if (current.cross.x >= block.minimum.x && target.cross.x <= block.minimum.x) // check for left edge
                {
                    attempt.result = { block.minimum.x, block.maximum.y };
                    attempt.edge = -1;
                    attempt.collideFloor = true;
                }
                if (current.cross.x <= block.maximum.x && target.cross.x >= block.maximum.x) // check for right edge
                {
                    attempt.result = { block.maximum.x, block.maximum.y };
                    attempt.edge = +1;
                    attempt.collideFloor = true;
                }
            }
        }

        if (testTargetCross.x == 0)
        {
            if (testCurrentMin.y != -1 && testTargetMin.y != +1) // test bottom into floor
            {
                attempt.result.y = maths::max(attempt.result.y, block.maximum.y);
                attempt.collideFloor = true;
            }
            if (testCurrentMax.y != +1 && testTargetMax.y != -1) // test top into ceiling
            {
                attempt.result.y = maths::min(attempt.result.y, block.minimum.y - current.height());
                attempt.collideCeiling = true;
            }
        }

        if (testTargetCross.y == 0)
        {
            if (testCurrentMin.x != -1 && testTargetMin.x != +1) // test left into wall
            {
                attempt.result.x = maths::max(attempt.result.x, block.maximum.x + target.widthLeft());
                attempt.collideWall = true;
            }
            if (testCurrentMax.x != +1 && testTargetMax.x != -1) // test right into wall
            {
                attempt.result.x = maths::min(attempt.result.x, block.minimum.x - target.widthRight());
                attempt.collideWall = true;
            }
        }

        // todo: this can probably be cleaned up and optimised

        if (edgeStop == false)
        {
            if (testTargetCross.x == +1 && testTargetCross.y == +1 && testTargetMin.x != +1 && testTargetMin.y != +1)
            {
                const Vec2F corner = { block.maximum.x, block.maximum.y };
                const Vec2F point = { target.cross.x, target.min.y };

                const float offsetCorner = maths::dot(normLeftDown, corner);
                const float offsetPoint = maths::dot(normLeftDown, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::max(attempt.result.x, attempt.result.x + normLeftDown.x * difference);
                    attempt.result.y = maths::max(attempt.result.y, attempt.result.y + normLeftDown.y * difference);
                    attempt.collideCorner = true;
                }
            }

            if (testTargetCross.x == -1 && testTargetCross.y == +1 && testTargetMax.x != -1 && testTargetMin.y != +1)
            {
                const Vec2F corner = { block.minimum.x, block.maximum.y };
                const Vec2F point = { target.cross.x, target.min.y };

                const float offsetCorner = maths::dot(normRightDown, corner);
                const float offsetPoint = maths::dot(normRightDown, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::min(attempt.result.x, attempt.result.x + normRightDown.x * difference);
                    attempt.result.y = maths::max(attempt.result.y, attempt.result.y + normRightDown.y * difference);
                    attempt.collideCorner = true;
                }
            }

            if (testTargetCross.x == +1 && testTargetCross.y == -1 && testTargetMin.x != +1 && testTargetMax.y != -1)
            {
                const Vec2F corner = { block.maximum.x, block.minimum.y };
                const Vec2F point = { target.cross.x, target.max.y };

                const float offsetCorner = maths::dot(normLeftUp, corner);
                const float offsetPoint = maths::dot(normLeftUp, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::max(attempt.result.x, attempt.result.x + normLeftUp.x * difference);
                    attempt.result.y = maths::min(attempt.result.y, attempt.result.y + normLeftUp.y * difference);
                    attempt.collideCorner = true;
                }
            }

            if (testTargetCross.x == -1 && testTargetCross.y == -1 && testTargetMax.x != -1 && testTargetMax.y != -1)
            {
                const Vec2F corner = { block.minimum.x, block.minimum.y };
                const Vec2F point = { target.cross.x, target.max.y };

                const float offsetCorner = maths::dot(normRightUp, corner);
                const float offsetPoint = maths::dot(normRightUp, point);

                if (const float difference = offsetCorner - offsetPoint; difference < 0.f)
                {
                    attempt.result.x = maths::min(attempt.result.x, attempt.result.x + normRightUp.x * difference);
                    attempt.result.y = maths::min(attempt.result.y, attempt.result.y + normRightUp.y * difference);
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
            if (current.min.y >= platform.originY && target.min.y <= platform.originY)
            {
                if (edgeStop == true)
                {
                    if (current.cross.x >= platform.minX && target.cross.x <= platform.minX)
                    {
                        attempt.result = { platform.minX, platform.originY };
                        attempt.edge = -1;
                        attempt.collideFloor = true;
                        attempt.onPlatform = true;
                    }
                    if (current.cross.x <= platform.maxX && target.cross.x >= platform.maxX)
                    {
                        attempt.result = { platform.maxX, platform.originY };
                        attempt.edge = +1;
                        attempt.collideFloor = true;
                        attempt.onPlatform = true;
                    }
                }

                if (target.cross.x >= platform.minX && target.cross.x <= platform.maxX)
                {
                    attempt.result.y = maths::max(attempt.result.y, platform.originY);
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

Ledge* Stage::find_ledge(const Diamond& diamond, int8_t facing, int8_t inputX)
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
        if (ledge.position.y <= diamond.min.y)
            continue;

        // fighter is on the inside of the ledge
        if (float(ledge.direction) * (diamond.cross.x - ledge.position.x) <= 0.f)
            continue;

        // todo: replace with an intersection test
        const Vec2F nearest = diamond.cross;
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
