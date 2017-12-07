// GLSL Fragment Shader

//============================================================================//

in float opacity;
in float index;
in float depth;

layout(binding=0) uniform sampler2DArray tex_Particles;

out vec4 frag_Colour;

//============================================================================//

void main()
{
    frag_Colour = texture(tex_Particles, vec3(gl_PointCoord, index));
    frag_Colour.a *= opacity;
}
