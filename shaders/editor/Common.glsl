// common functions for cubemap filtering

//============================================================================//

// convert ndc xy and face index to a cubemap lookup vector

vec3 get_direction(vec2 ndc, int face)
{
    vec3 result = vec3(0.0, 0.0, 0.0);

    if (face == 0) result = vec3(+1.0, -ndc.y, -ndc.x);
    if (face == 1) result = vec3(-1.0, -ndc.y, +ndc.x);
    if (face == 2) result = vec3(+ndc.x, +1.0, +ndc.y);
    if (face == 3) result = vec3(+ndc.x, -1.0, -ndc.y);
    if (face == 4) result = vec3(+ndc.x, -ndc.y, +1.0);
    if (face == 5) result = vec3(-ndc.x, -ndc.y, -1.0);

    return normalize(result);
}

//============================================================================//

// calculate area weight of a texel (solid angle)

float area_element(float ndcX, float ndcY)
{
    const float y = ndcX * ndcY;
    const float x = sqrt(ndcX * ndcX + ndcY * ndcY + 1.0);

    // atan2 https://stackoverflow.com/a/26070411/3326595
    return abs(x) > abs(y) ? atan(y,x) : (1.570796327 - atan(x,y));
}

float get_texel_area(vec2 ndc)
{
    const float CORNER_OFFSET = 1.0 / float(SOURCE_SIZE);

    const vec2 cornerNeg = ndc - CORNER_OFFSET;
    const vec2 cornerPos = ndc + CORNER_OFFSET;

    const float areaA = area_element(cornerNeg.x, cornerNeg.y);
    const float areaB = area_element(cornerNeg.x, cornerPos.y);
    const float areaC = area_element(cornerPos.x, cornerNeg.y);
    const float areaD = area_element(cornerPos.x, cornerPos.y);

    return areaA - areaB - areaC + areaD;
}
