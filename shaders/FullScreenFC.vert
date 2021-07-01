#version 450

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    const ivec2 texCoord = ivec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);
}
