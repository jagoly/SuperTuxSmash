// GLSL Vertex Shader

#include headers/blocks/Camera

//============================================================================//

// render a full screen quad with a bufferless VAO

const vec2 V_pos_data[4] = { vec2(-1, -1), vec2(-1, +1), vec2(+1, -1), vec2(+1, +1) };
vec2 V_pos = V_pos_data[gl_VertexID];

//============================================================================//

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };

out vec3 v_cube_norm;

out gl_PerVertex { vec4 gl_Position; };

//============================================================================//

void main()
{
    vec3 viewpos = vec3(CB.proj_inv * vec4(V_pos, 0.f, 1.f));
    v_cube_norm = normalize(mat3(CB.view_inv) * viewpos);
    gl_Position = vec4(V_pos, 0.f, 1.f);
}
