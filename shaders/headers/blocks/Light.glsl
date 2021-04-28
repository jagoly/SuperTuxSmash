// GLSL Uniform Block

layout(std140, set=1, binding=0) uniform LightBlock
{
    vec3 ambiColour;
    vec3 skyColour;
    vec3 skyDirection;
    mat4 skyMatrix;
}
LB;
