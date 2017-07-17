// GLSL Vertex Shader

//============================================================================//

layout(location=0) in vec3 V_pos;

layout(location=0) uniform mat4 u_final_mat;

//============================================================================//

void main()
{
    gl_Position = u_final_mat * vec4(V_pos, 1.f);
}
