// GLSL Fragment Shader

//============================================================================//

in vec2 texcrd;

layout(binding=0) uniform sampler2D tex;

out vec4 fragColour;

//============================================================================//

void main()
{
    fragColour = texture(tex, texcrd);
}
