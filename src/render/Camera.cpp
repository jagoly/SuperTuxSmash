#include "render/Camera.hpp"

#include <sqee/maths/Functions.hpp>

using namespace sts;

//============================================================================//

Camera::Camera(const Renderer& renderer) : renderer(renderer) {}

Camera::~Camera() = default;

//============================================================================//

Mat4F Camera::compute_light_matrix(const Mat4F& lightViewMat, Vec3F stageMin, Vec3F stageMax) const
{
    // todo: this can almost certainly be done better or easier, but it works for now

    constexpr float CAMERA_NEAR = 0.5f;

    const Mat4F invProjViewMat = maths::inverse(mBlock.projViewMat);

    const auto compute_frustum_corner = [&invProjViewMat](Vec4F clipPos)
    {
        const Vec4F worldPosW = invProjViewMat * clipPos;
        return Vec3F(worldPosW) / worldPosW.w;
    };

    struct Segment { Vec3F a, b; };

    const Vec3F nearA = compute_frustum_corner(Vec4F(-1.f, -1.f, 0.f, 1.f));
    const Vec3F nearB = compute_frustum_corner(Vec4F(+1.f, -1.f, 0.f, 1.f));
    const Vec3F nearC = compute_frustum_corner(Vec4F(+1.f, +1.f, 0.f, 1.f));
    const Vec3F nearD = compute_frustum_corner(Vec4F(-1.f, +1.f, 0.f, 1.f));

    const Vec3F farA = compute_frustum_corner(Vec4F(-1.f, -1.f, 1.f, 1.f));
    const Vec3F farB = compute_frustum_corner(Vec4F(+1.f, -1.f, 1.f, 1.f));
    const Vec3F farC = compute_frustum_corner(Vec4F(+1.f, +1.f, 1.f, 1.f));
    const Vec3F farD = compute_frustum_corner(Vec4F(-1.f, +1.f, 1.f, 1.f));

    const Vec3F normalA = maths::normalize(maths::cross(extract_position() - farA, extract_position() - farB));
    const Vec3F normalB = maths::normalize(maths::cross(extract_position() - farB, extract_position() - farC));
    const Vec3F normalC = maths::normalize(maths::cross(extract_position() - farC, extract_position() - farD));
    const Vec3F normalD = maths::normalize(maths::cross(extract_position() - farD, extract_position() - farA));

    //--------------------------------------------------------//

    const std::array<Vec4F, 6u> stagePlanes =
    {
        Vec4F(+1.f, 0.f, 0.f, -stageMin.x),
        Vec4F(-1.f, 0.f, 0.f, +stageMax.x),
        Vec4F(0.f, +1.f, 0.f, -stageMin.y),
        Vec4F(0.f, -1.f, 0.f, +stageMax.y),
        Vec4F(0.f, 0.f, +1.f, -stageMin.z),
        Vec4F(0.f, 0.f, -1.f, +stageMax.z)
    };

    const std::array<Vec4F, 5u> cameraPlanes =
    {
        Vec4F(extract_direction(), maths::dot(-extract_direction(), extract_position() + extract_direction() * CAMERA_NEAR)),
        Vec4F(normalA, maths::dot(-normalA, extract_position())),
        Vec4F(normalB, maths::dot(-normalB, extract_position())),
        Vec4F(normalC, maths::dot(-normalC, extract_position())),
        Vec4F(normalD, maths::dot(-normalD, extract_position()))
    };

    StackVector<Segment, 12u> stageSegments =
    {
        Segment { { stageMin.x, stageMin.y, stageMin.z }, { stageMax.x, stageMin.y, stageMin.z } },
        Segment { { stageMin.x, stageMin.y, stageMax.z }, { stageMax.x, stageMin.y, stageMax.z } },
        Segment { { stageMin.x, stageMax.y, stageMin.z }, { stageMax.x, stageMax.y, stageMin.z } },
        Segment { { stageMin.x, stageMax.y, stageMax.z }, { stageMax.x, stageMax.y, stageMax.z } },
        Segment { { stageMin.x, stageMin.y, stageMin.z }, { stageMin.x, stageMax.y, stageMin.z } },
        Segment { { stageMin.x, stageMin.y, stageMax.z }, { stageMin.x, stageMax.y, stageMax.z } },
        Segment { { stageMax.x, stageMin.y, stageMin.z }, { stageMax.x, stageMax.y, stageMin.z } },
        Segment { { stageMax.x, stageMin.y, stageMax.z }, { stageMax.x, stageMax.y, stageMax.z } },
        Segment { { stageMin.x, stageMin.y, stageMin.z }, { stageMin.x, stageMin.y, stageMax.z } },
        Segment { { stageMin.x, stageMax.y, stageMin.z }, { stageMin.x, stageMax.y, stageMax.z } },
        Segment { { stageMax.x, stageMin.y, stageMin.z }, { stageMax.x, stageMin.y, stageMax.z } },
        Segment { { stageMax.x, stageMax.y, stageMin.z }, { stageMax.x, stageMax.y, stageMax.z } }
    };

    StackVector<Segment, 12u> cameraSegments =
    {
        Segment { nearA, farA },
        Segment { nearB, farB },
        Segment { nearC, farC },
        Segment { nearD, farD },
        Segment { nearA, nearB },
        Segment { nearB, nearC },
        Segment { nearC, nearD },
        Segment { nearD, nearA },
        Segment { farA, farB },
        Segment { farB, farC },
        Segment { farC, farD },
        Segment { farD, farA }
    };

    //--------------------------------------------------------//

    const auto clip_to_plane = [](Vec4F plane, Vec3F& p1, Vec3F& p2) -> bool
    {
        const float d1 = maths::dot(Vec3F(plane), p1) + plane.w;
        const float d2 = maths::dot(Vec3F(plane), p2) + plane.w;

        if (d1 < 0.f && d2 < 0.f) return true; // discard segment

        // first point needs to be clipped
        if (d1 < 0.f/* && d2 > 0.001f*/) p1 = p1 + (p2 - p1) * (d1 / (d1 - d2));

        // second point needs to be clipped
        if (d2 < 0.f/* && d1 > 0.001f*/) p2 = p1 + (p2 - p1) * (d1 / (d1 - d2));

        return false;
    };

    for (const auto& plane : cameraPlanes)
    {
        for (auto iter = stageSegments.begin(); iter != stageSegments.end();)
        {
            if (clip_to_plane(plane, iter->a, iter->b) == true)
                iter = stageSegments.erase(iter);
            else ++iter;
        }
    }

    for (const auto& plane : stagePlanes)
    {
        for (auto iter = cameraSegments.begin(); iter != cameraSegments.end();)
        {
            if (clip_to_plane(plane, iter->a, iter->b) == true)
                iter = cameraSegments.erase(iter);
            else ++iter;
        }
    }

    //--------------------------------------------------------//

    Vec3F worldMin = Vec3F(+INFINITY);
    Vec3F worldMax = Vec3F(-INFINITY);

    for (const Segment& segment : stageSegments)
    {
        worldMin = maths::min(worldMin, segment.a);
        worldMin = maths::min(worldMin, segment.b);
        worldMax = maths::max(worldMax, segment.a);
        worldMax = maths::max(worldMax, segment.b);
    }

    for (const Segment& segment : cameraSegments)
    {
        worldMin = maths::min(worldMin, segment.a);
        worldMin = maths::min(worldMin, segment.b);
        worldMax = maths::max(worldMax, segment.a);
        worldMax = maths::max(worldMax, segment.b);
    }

    //--------------------------------------------------------//

    std::array<Vec3F, 8u> corners;
    corners[0] = { worldMin.x, worldMin.y, worldMin.z };
    corners[1] = { worldMin.x, worldMin.y, worldMax.z };
    corners[2] = { worldMin.x, worldMax.y, worldMin.z };
    corners[3] = { worldMin.x, worldMax.y, worldMax.z };
    corners[4] = { worldMax.x, worldMin.y, worldMin.z };
    corners[5] = { worldMax.x, worldMin.y, worldMax.z };
    corners[6] = { worldMax.x, worldMax.y, worldMin.z };
    corners[7] = { worldMax.x, worldMax.y, worldMax.z };

    Vec3F orthoMin = Vec3F(+INFINITY);
    Vec3F orthoMax = Vec3F(-INFINITY);

    for (const Vec3F& corner : corners)
    {
        const Vec3F lightPos = Vec3F(lightViewMat * Vec4F(corner, 1.f));

        orthoMin = maths::min(orthoMin, lightPos);
        orthoMax = maths::max(orthoMax, lightPos);
    }

    return maths::ortho_LH(orthoMin, orthoMax) * lightViewMat;
}
