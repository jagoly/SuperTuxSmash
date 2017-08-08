// GLSL Vertex Shader

//============================================================================//

layout(location=0) in vec3 v_Position;
layout(location=1) in vec2 v_TexCoord;

layout(location=0) uniform mat4 u_Matrix;

out vec2 texcrd;

//============================================================================//

void main() 
{
    texcrd = v_TexCoord;

    gl_Position = u_Matrix * vec4(v_Position, 1.f);
}
