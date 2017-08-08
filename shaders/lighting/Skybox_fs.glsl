// GLSL Fragment Shader

//============================================================================//

in vec3 cubeNorm;

layout(binding=0) uniform samplerCube tex_Skybox;

out vec3 frag_Colour;

//============================================================================//

void main()
{
    frag_Colour = texture(tex_Skybox, cubeNorm).rgb;
}
