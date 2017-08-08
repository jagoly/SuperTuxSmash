// GLSL Fragment Shader

//============================================================================//

layout(location=1) uniform vec3 u_Colour;

layout(location=0) out vec4 frag_Colour;

//============================================================================//

void main() 
{
    frag_Colour = vec4(u_Colour, 0.25f);
}
