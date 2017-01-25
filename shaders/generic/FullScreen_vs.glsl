// GLSL Vertex Shader

//============================================================================//

// render a full screen quad with a bufferless VAO

const vec2 V_pos_data[4] = { vec2(-1, -1), vec2(-1, +1), vec2(+1, -1), vec2(+1, +1) };
const vec2 V_pos = V_pos_data[gl_VertexID];

//============================================================================//

out vec2 texcrd;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main()
{
    texcrd = V_pos * 0.5f + 0.5f;
    gl_Position = vec4(V_pos, 0.f, 1.f);
}
