// GLSL Fragment Shader

//============================================================================//

in vec2 texcrd;

layout(binding=0) uniform sampler2D tex_Mask;

//============================================================================//

void main()
{
    if (texture(tex_Mask, texcrd).a < 0.5f) discard;
}
