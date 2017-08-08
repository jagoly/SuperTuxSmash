// GLSL Vertex Shader

//============================================================================//

#include builtin/misc/screen

out vec2 texcrd;

//============================================================================//

void main()
{
    texcrd = v_Position * 0.5f + 0.5f;

    gl_Position = vec4(v_Position, 0.f, 1.f);
}
