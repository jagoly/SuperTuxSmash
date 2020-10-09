// GLSL Vertex Shader

//============================================================================//

//#include runtime/Options
#include headers/blocks/Camera

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in float v_Radius;
layout(location=2) in vec3 v_Colour;
layout(location=3) in float v_Opacity;
layout(location=4) in float v_Misc;
layout(location=5) in float v_Index;

out VertexBlock {
 vec3 viewPos;
 float radius;
 vec3 colour;
 float opacity;
 float index;
} OUT;

//============================================================================//

void main() 
{
    OUT.viewPos = vec3(CB.viewMat * vec4(v_Position, 1.f));
    OUT.colour = v_Colour;
    OUT.opacity = v_Opacity;
    OUT.radius = v_Radius;
    OUT.index = v_Index;
    
    //const vec4 viewPos = CB.viewMat * vec4(v_Position, 1.f);
    //const vec4 projVoxel = CB.projMat * vec4(v_Radius, v_Radius, viewPos.z, viewPos.w);
    //const vec2 projSize = OPTION_WinSizeFull * projVoxel.xy / projVoxel.w;

    //gl_PointSize = 0.25 * (projSize.x + projSize.y);

//    gl_Position = CB.projMat * vec4(OUT.viewPos, 1.f);
}
