// GLSL Fragment Shader

//============================================================================//

#include runtime/Options
#include headers/blocks/Camera

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };

//============================================================================//

in GeometryBlock {
 vec3 texcrd;
 float depth;
 vec3 colour;
 float opacity;
 float radius;
} IN;

layout(binding=0) uniform sampler2DArray tex_Particles;
layout(binding=1) uniform sampler2D tex_Depth;

out vec4 frag_Colour;

//============================================================================//

float get_depth_distance()
{
    const vec2 fragcrd = gl_FragCoord.xy / OPTION_WinSizeFull;
    
    const float depth = texture(tex_Depth, fragcrd).r * 2.f - 1.f;
    const vec4 projPos = vec4(fragcrd * 2.f - 1.f, depth, 1.f);
    const vec4 viewPosW = CB.invProjMat * projPos;

    return viewPosW.z / viewPosW.w;
}

void main()
{
    frag_Colour = texture(tex_Particles, IN.texcrd);
    frag_Colour *= vec4(IN.colour, IN.opacity);
    
    const float depthDiff = IN.depth - get_depth_distance();
    //if (IN.depth > get_depth_distance()) discard;
    
    frag_Colour.a *= clamp(IN.radius - depthDiff, 0.f, 1.f);
}
