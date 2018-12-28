// GLSL Vertex Shader

//============================================================================//

#include runtime/Options
#include headers/blocks/Camera

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };

//============================================================================//1

layout(location=0) in vec3 v_Position;
layout(location=1) in float v_Radius;
layout(location=2) in vec3 v_Colour;
layout(location=3) in float v_Opacity;
layout(location=4) in float v_Misc;
layout(location=5) in float v_Index;

out vec3 colour;
out float opacity;
out float index;

out float depth;

//============================================================================//

void main() 
{
    colour = v_Colour;
    opacity = v_Opacity;
    index = v_Index;
    
    vec4 eyePos = CB.viewMat * vec4(v_Position, 1.f);
    vec4 projVoxel = CB.projMat * vec4(v_Radius, v_Radius, eyePos.z, eyePos.w);
    vec2 projSize = OPTION_WinSizeFull * projVoxel.xy / projVoxel.w;
    
    gl_PointSize = 0.25 * (projSize.x + projSize.y);

    gl_Position = CB.projMat * eyePos;
    
    depth = v_Position.z;
    //gl_Position = vec4(v_Position.xy, 0.f, 1.f);
}
