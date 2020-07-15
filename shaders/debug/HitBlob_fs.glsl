// GLSL Fragment Shader

//============================================================================//

layout(location=1) uniform vec3 u_Colour;
layout(location=2) uniform float u_Opacity;

layout(location=0) out vec4 frag_Colour;

//============================================================================//

void main() 
{
    frag_Colour = vec4(u_Colour, u_Opacity);
}
