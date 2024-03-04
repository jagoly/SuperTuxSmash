#version 450

layout(location=0) in vec4 v_Position;
layout(location=1) in vec4 v_Colour;

layout(location=0) out vec4 io_Colour;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    io_Colour = v_Colour;
    gl_Position = v_Position;
}
