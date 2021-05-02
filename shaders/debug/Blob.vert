#version 450

layout(push_constant) uniform PushConstants
{
    layout(offset=0) mat4 matrix;
}
PC;

layout(location=0) in vec3 v_Position;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = PC.matrix * vec4(v_Position, 1.f);
}
