SKIP: FAILED

#version 310 es

layout(rg32ui) uniform highp writeonly uimage2DArray arg_0;
layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec4 inner;
} prevent_dce;

void textureLoad_54fb38() {
  uvec2 arg_1 = uvec2(1u);
  uint arg_2 = 1u;
  uvec4 res = imageLoad(arg_0, ivec3(uvec3(arg_1, arg_2)));
  prevent_dce.inner = res;
}

vec4 vertex_main() {
  textureLoad_54fb38();
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
Error parsing GLSL shader:
ERROR: 0:3: 'image load-store format' : not supported with this profile: es
ERROR: 0:3: '' : compilation terminated 
ERROR: 2 compilation errors.  No code generated.



#version 310 es
precision highp float;

layout(rg32ui) uniform highp writeonly uimage2DArray arg_0;
layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec4 inner;
} prevent_dce;

void textureLoad_54fb38() {
  uvec2 arg_1 = uvec2(1u);
  uint arg_2 = 1u;
  uvec4 res = imageLoad(arg_0, ivec3(uvec3(arg_1, arg_2)));
  prevent_dce.inner = res;
}

void fragment_main() {
  textureLoad_54fb38();
}

void main() {
  fragment_main();
  return;
}
Error parsing GLSL shader:
ERROR: 0:4: 'image load-store format' : not supported with this profile: es
ERROR: 0:4: '' : compilation terminated 
ERROR: 2 compilation errors.  No code generated.



#version 310 es

layout(rg32ui) uniform highp writeonly uimage2DArray arg_0;
layout(binding = 0, std430) buffer prevent_dce_block_ssbo {
  uvec4 inner;
} prevent_dce;

void textureLoad_54fb38() {
  uvec2 arg_1 = uvec2(1u);
  uint arg_2 = 1u;
  uvec4 res = imageLoad(arg_0, ivec3(uvec3(arg_1, arg_2)));
  prevent_dce.inner = res;
}

void compute_main() {
  textureLoad_54fb38();
}

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  compute_main();
  return;
}
Error parsing GLSL shader:
ERROR: 0:3: 'image load-store format' : not supported with this profile: es
ERROR: 0:3: '' : compilation terminated 
ERROR: 2 compilation errors.  No code generated.



