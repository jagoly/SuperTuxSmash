// GLSL Fragment Shader

//============================================================================//

in vec3 v_cube_norm;

layout(binding=0) uniform samplerCube tex_skybox;

out vec3 frag_colour;

//============================================================================//

void main()
{
    frag_colour = texture(tex_skybox, v_cube_norm).rgb;

//    frag_colour = v_cube_norm;
//    frag_colour = vec3(0.5, 0.1, 0.1);
}
