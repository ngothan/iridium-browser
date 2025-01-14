#version 310 es

uniform highp sampler2DArrayShadow arg_0_arg_1;

layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  float inner;
} prevent_dce;

void textureSampleLevel_1bf73e() {
  vec2 arg_2 = vec2(1.0f);
  int arg_3 = 1;
  int arg_4 = 1;
  float res = textureLod(arg_0_arg_1, vec4(vec3(arg_2, float(arg_3)), 0.0f), float(arg_4));
  prevent_dce.inner = res;
}

vec4 vertex_main() {
  textureSampleLevel_1bf73e();
  return vec4(0.0f);
}

void main() {
  gl_PointSize = 1.0;
  vec4 inner_result = vertex_main();
  gl_Position = inner_result;
  gl_Position.y = -(gl_Position.y);
  gl_Position.z = ((2.0f * gl_Position.z) - gl_Position.w);
  return;
}
#version 310 es
precision highp float;

uniform highp sampler2DArrayShadow arg_0_arg_1;

layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  float inner;
} prevent_dce;

void textureSampleLevel_1bf73e() {
  vec2 arg_2 = vec2(1.0f);
  int arg_3 = 1;
  int arg_4 = 1;
  float res = textureLod(arg_0_arg_1, vec4(vec3(arg_2, float(arg_3)), 0.0f), float(arg_4));
  prevent_dce.inner = res;
}

void fragment_main() {
  textureSampleLevel_1bf73e();
}

void main() {
  fragment_main();
  return;
}
#version 310 es

uniform highp sampler2DArrayShadow arg_0_arg_1;

layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  float inner;
} prevent_dce;

void textureSampleLevel_1bf73e() {
  vec2 arg_2 = vec2(1.0f);
  int arg_3 = 1;
  int arg_4 = 1;
  float res = textureLod(arg_0_arg_1, vec4(vec3(arg_2, float(arg_3)), 0.0f), float(arg_4));
  prevent_dce.inner = res;
}

void compute_main() {
  textureSampleLevel_1bf73e();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
