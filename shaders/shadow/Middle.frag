#version 450

layout(set=0, binding=0, input_attachment_index=0) uniform subpassInput sp_Front;
layout(set=0, binding=1, input_attachment_index=1) uniform subpassInput sp_Back;

out float gl_FragDepth;

void main()
{
    const float front = subpassLoad(sp_Front).r;
    const float back = subpassLoad(sp_Back).r;

    gl_FragDepth = max((front + back) * 0.5, front + 0.002);
    //gl_FragDepth = back;
}
