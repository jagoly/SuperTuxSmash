// GLSL Fragment Shader

//============================================================================//

layout(location=1) uniform vec3 colour;

layout(location=0) out vec4 frag_colour;

//============================================================================//

void main() 
{
    frag_colour = vec4(colour, 0.25f);
}
