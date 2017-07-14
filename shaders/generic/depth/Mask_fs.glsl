// GLSL Fragment Shader

//============================================================================//

in vec2 texcrd;

//============================================================================//

layout(binding=0) uniform sampler2D tex_mask;

//============================================================================//

void main() 
{
    if (texture(tex_mask, texcrd).a < 0.5f) discard;
}