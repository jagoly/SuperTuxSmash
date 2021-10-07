// GLSL Fragment Super Shader

//============================================================================//

// TODO: options

//============================================================================//

layout(set=1, binding=1) uniform sampler2D tx_Colour;

layout(location=0) in vec3 io_ViewPos;
layout(location=1) in vec2 io_TexCoord;
layout(location=2) in vec4 io_Params;

layout(location=0) out vec4 frag_RGBA;

//============================================================================//

void main()
{
    frag_RGBA = texture(tx_Colour, io_TexCoord) * io_Params;
    //frag_RGBA.gba = vec3(1,1,1);
}
