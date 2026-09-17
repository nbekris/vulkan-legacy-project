
#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "shared_structs.h"

layout(push_constant) uniform _PushConstantPost
{
  PushConstantPost pcPost;
};


layout(location = 0) out vec4 fragColor;


void main()
{
    vec2 uv = gl_FragCoord.xy/vec2(pcPost.width, pcPost.height);
    fragColor = vec4(uv, 0, 1);
}
