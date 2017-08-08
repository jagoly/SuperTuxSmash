// GLSL Vertex Shader

//============================================================================//

layout(location=0) in vec3 v_Position;

layout(location=0) uniform mat4 u_Matrix;

//============================================================================//

void main()
{
    gl_Position = u_Matrix * vec4(v_Position, 1.f);
}
