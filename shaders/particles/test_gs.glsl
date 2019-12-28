// GLSL Geometry Shader

//============================================================================//

#include runtime/Options
#include headers/blocks/Camera

layout(std140, binding=0) uniform CAMERABLOCK { CameraBlock CB; };

//============================================================================//

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;
   
// GLSL Hacker automatic uniforms:
uniform mat4 gxl3d_ViewProjectionMatrix;
uniform mat4 gxl3d_ModelViewMatrix;

in VertexBlock {
 vec3 viewPos;
 float radius;
 vec3 colour;
 float opacity;
 float index;
} IN[];

out GeometryBlock {
 vec3 texcrd;
 float depth;
 vec3 colour;
 float opacity;
 float radius;
} OUT;
   
void main()
{
  //const vec3 offsX = vec3(CB.viewMat[0][0], CB.viewMat[1][0], CB.viewMat[2][0]);
  //const vec3 offsY = vec3(CB.viewMat[0][1], CB.viewMat[1][1], CB.viewMat[2][1]);
  const vec3 offsX = vec3(IN[0].radius, 0.f, 0.f);
  const vec3 offsY = vec3(0.f, IN[0].radius, 0.f);

  const vec3 viewPosA = IN[0].viewPos + (-offsX -offsY) * IN[0].radius;
  const vec3 viewPosB = IN[0].viewPos + (-offsX +offsY) * IN[0].radius;
  const vec3 viewPosC = IN[0].viewPos + (+offsX -offsY) * IN[0].radius;
  const vec3 viewPosD = IN[0].viewPos + (+offsX +offsY) * IN[0].radius;
  
  OUT.texcrd = vec3(0.f, 0.f, IN[0].index);
  OUT.depth = viewPosA.z;
  OUT.colour = IN[0].colour;
  OUT.opacity = IN[0].opacity;
  OUT.radius = IN[0].radius;
  gl_Position = CB.projMat * vec4(viewPosA, 1.f);
  EmitVertex();

  OUT.texcrd = vec3(0.f, 1.f, IN[0].index);
  OUT.depth = viewPosB.z;
  OUT.colour = IN[0].colour;
  OUT.opacity = IN[0].opacity;
  OUT.radius = IN[0].radius;
  gl_Position = CB.projMat * vec4(viewPosB, 1.f);
  EmitVertex();
  
  OUT.texcrd = vec3(1.f, 0.f, IN[0].index);
  OUT.depth = viewPosC.z;
  OUT.colour = IN[0].colour;
  OUT.opacity = IN[0].opacity;
  OUT.radius = IN[0].radius;
  gl_Position = CB.projMat * vec4(viewPosC, 1.f);
  EmitVertex();
  
  OUT.texcrd = vec3(1.f, 1.f, IN[0].index);
  OUT.depth = viewPosD.z;
  OUT.colour = IN[0].colour;
  OUT.opacity = IN[0].opacity;
  OUT.radius = IN[0].radius;
  gl_Position = CB.projMat * vec4(viewPosD, 1.f);
  EmitVertex();

  EndPrimitive();  
}   
