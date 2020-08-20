// GLSL Fragment Shader

//============================================================================//

layout(location=0) uniform vec3 u_Colour;

layout(location=0) out vec4 frag_Colour;

//============================================================================//

void main() 
{
    frag_Colour = vec4(u_Colour, 1.f);
}
