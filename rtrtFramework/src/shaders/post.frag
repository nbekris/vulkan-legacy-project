
#version 450
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "shared_structs.h"

layout(push_constant) uniform _PushConstantPost
{
  PushConstantPost pcPost;
};


layout(location = 0) out vec4 fragColor;
layout(set=0, binding=0) uniform sampler2D renderTarget;


void main()
{
    vec2 uv = gl_FragCoord.xy/vec2(pcPost.width, pcPost.height);
    //fragColor = vec4(uv, 0, 1);
    fragColor = pow(texture(renderTarget, uv), vec4(1.0/2.2));
}
