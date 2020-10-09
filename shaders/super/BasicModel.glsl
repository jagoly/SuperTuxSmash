// GLSL Combined Super Shader

//============================================================================//

#option APPLY_SKELETON 0

#option DIFFUSE_TEXTURE 0
#option SPECULAR_TEXTURE 0
#option MASK_TEXTURE 0
#option NORMAL_TEXTURE 0

#option FAKE_SUBSURF_SCATTER 0

//----------------------------------------------------------------------------//

#if DIFFUSE_TEXTURE || SPECULAR_TEXTURE || MASK_TEXTURE || NORMAL_TEXTURE
  #define VERTEX_TEXCOORD 1
#endif

#if NORMAL_TEXTURE
  #define VERTEX_TANGENT 1
#endif

#if APPLY_SKELETON
  #define VERTEX_BONES 1
#endif

//----------------------------------------------------------------------------//

#if DIFFUSE_TEXTURE
  layout(location=0, binding=0) uniform sampler2D tx_Diffuse;
#else
  layout(location=0) uniform vec3 uf_Diffuse;
#endif

#if SPECULAR_TEXTURE
  layout(location=1, binding=1) uniform sampler2D tx_Specular;
#else
  layout(location=1) uniform vec3 uf_Specular;
#endif

#if MASK_TEXTURE
  layout(location=2, binding=2) uniform sampler2D tx_Mask;
#endif

#if NORMAL_TEXTURE
  layout(location=3, binding=3) uniform sampler2D tx_Normal;
#endif

//----------------------------------------------------------------------------//

#include headers/blocks/Camera
#include headers/blocks/Light

#if APPLY_SKELETON
  #include headers/blocks/Skelly
#else
  #include headers/blocks/Static
#endif

#line 61

//============================================================================//

#if defined(SHADER_STAGE_VERTEX)

//----------------------------------------------------------------------------//

layout(location=0) in vec3 v_Position;

#if VERTEX_TEXCOORD
  layout(location=1) in vec2 v_TexCoord;
#endif

layout(location=2) in vec3 v_Normal;

#if VERTEX_TANGENT
  layout(location=3) in vec4 v_Tangent;
#endif

#if VERTEX_BONES
  layout(location=5) in ivec4 v_Bones;
  layout(location=6) in vec4 v_Weights;
#endif

layout(location=0) out vec3 io_ViewPos;

#if VERTEX_TEXCOORD
  layout(location=1) out vec2 io_TexCoord;
#endif

layout(location=2) out vec3 io_Normal;

#if VERTEX_TANGENT
  layout(location=3) out vec3 io_Tangent;
  layout(location=4) out vec3 io_Bitangent;
#endif

//----------------------------------------------------------------------------//

void main() 
{
    #if APPLY_SKELETON

      const float weightSum = v_Weights.r + v_Weights.g + v_Weights.b + v_Weights.a;
      const vec4 weights = v_Weights * (1.f / weightSum);

      vec3 position                  = vec4(v_Position, 1.f) * SB.bones[v_Bones.r] * weights.r;
      if (v_Bones.g != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.g] * weights.g;
      if (v_Bones.b != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.b] * weights.b;
      if (v_Bones.a != -1) position += vec4(v_Position, 1.f) * SB.bones[v_Bones.a] * weights.a;

      vec3 normal                  = v_Normal * mat3(SB.bones[v_Bones.r]) * weights.r;
      if (v_Bones.g != -1) normal += v_Normal * mat3(SB.bones[v_Bones.g]) * weights.g;
      if (v_Bones.b != -1) normal += v_Normal * mat3(SB.bones[v_Bones.b]) * weights.b;
      if (v_Bones.a != -1) normal += v_Normal * mat3(SB.bones[v_Bones.a]) * weights.a;

      #if VERTEX_TANGENT
        vec3 tangent                  = v_Tangent.xyz * mat3(SB.bones[v_Bones.r]) * weights.r;
        if (v_Bones.g != -1) tangent += v_Tangent.xyz * mat3(SB.bones[v_Bones.g]) * weights.g;
        if (v_Bones.b != -1) tangent += v_Tangent.xyz * mat3(SB.bones[v_Bones.b]) * weights.b;
        if (v_Bones.a != -1) tangent += v_Tangent.xyz * mat3(SB.bones[v_Bones.a]) * weights.a;
      #endif

    #else // not APPLY_SKELETON

      const vec3 position = v_Position;

      const vec3 normal = v_Normal;

      #if VERTEX_TANGENT
        const vec3 tangent = v_Tangent;
      #endif

    #endif // APPLY_SKELETON

    io_ViewPos = vec3(CB.invProjMat * SB.matrix * vec4(position, 1.f));

    #if VERTEX_TEXCOORD
      io_TexCoord = v_TexCoord;
    #endif

    io_Normal = normalize(SB.normMat * normal);
    
    #if VERTEX_TANGENT
      io_Tangent = normalize(SB.normMat * tangent);
      io_Bitangent = normalize(SB.normMat * cross(normal, tangent) * v_Tangent.w);
    #endif

    gl_Position = SB.matrix * vec4(position, 1.f);
}

#endif // SHADER_STAGE_VERTEX

//============================================================================//

#if defined(SHADER_STAGE_FRAGMENT)

//----------------------------------------------------------------------------//

layout(location=0) in vec3 io_ViewPos;

#if VERTEX_TEXCOORD
  layout(location=1) in vec2 io_TexCoord;
#endif

layout(location=2) in vec3 io_Normal;

#if VERTEX_TANGENT
  layout(location=3) in vec3 io_Tangent;
  layout(location=4) in vec3 io_Bitangent;
#endif

layout(location=0) out vec3 frag_Colour;

//----------------------------------------------------------------------------//

vec3 get_diffuse_value(vec3 colour, vec3 lightDir, vec3 normal)
{
    float factor = dot(-lightDir, normal);
    
    #if FAKE_SUBSURF_SCATTER
      const float wrap = 0.5f;
      factor = (factor + wrap) / (1.f + wrap);
    #endif

    return colour * max(factor, 0.f);
}

vec3 get_specular_value(vec3 colour, float gloss, vec3 lightDir, vec3 normal)
{
    vec3 reflection = reflect(lightDir, normal);
    vec3 dirFromCam = normalize(-io_ViewPos);

    float factor = max(dot(dirFromCam, reflection), 0.f);
    factor = pow(factor, gloss * 100.f);

    return colour * factor;
}

//----------------------------------------------------------------------------//

void main()
{
    #if MASK_TEXTURE
      if (texture(tx_Mask, io_TexCoord).a < 0.5f) discard;
    #endif

    #if DIFFUSE_TEXTURE
      const vec3 diffuse = texture(tx_Diffuse, io_TexCoord).rgb;
    #else
      const vec3 diffuse = uf_Diffuse;
    #endif

    #if SPECULAR_TEXTURE
      const vec3 specular = texture(tx_Specular, io_TexCoord).rgb;
    #else
      const vec3 specular = uf_Specular;
    #endif

    #if NORMAL_TEXTURE
      const vec3 nm = texture(tx_Normal, io_TexCoord).rgb;
      const vec3 T = io_Tangent * nm.x;
      const vec3 B = io_Bitangent * nm.y;
      const vec3 N = io_Normal * nm.z;
      const vec3 normal = normalize(T + B + N);
    #else
      const vec3 normal = normalize(io_Normal);
    #endif

    const vec3 lightDir = mat3(CB.viewMat) * normalize(LB.skyDirection);

    frag_Colour = vec3(0.f, 0.f, 0.f);
    
    frag_Colour += diffuse * LB.ambiColour;
    frag_Colour += get_diffuse_value(diffuse, lightDir, normal) * LB.skyColour;

    frag_Colour += get_specular_value(specular, 0.5f, lightDir, normal) * LB.skyColour;
}

#endif // SHADER_STAGE_FRAGMENT
