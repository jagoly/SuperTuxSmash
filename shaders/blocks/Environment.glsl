layout(std140, set=1, binding=0)
uniform EnvironmentBlock
{
    vec3 ambientColour;
    vec3 lightColour;
    vec3 lightDirection;
    mat4 lightMatrix;
}
ENV;
