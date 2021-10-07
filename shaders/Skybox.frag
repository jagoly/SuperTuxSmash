#version 450

layout(set=0, binding=1) uniform samplerCube tx_Skybox;

layout(location=0) in vec3 io_CubeNorm;

layout(location=0) out vec4 frag_Colour;

void main()
{
    frag_Colour = texture(tx_Skybox, io_CubeNorm);
}
