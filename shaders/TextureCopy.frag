// Precompiled SPIR-V binaries are embedded in VulkanExample.cpp, this is for refenrece.
// If you wish to compile the shader by yourself, use for e.g. glslangValidator from
// https://github.com/KhronosGroup/glslang and run
// $ glslangValidator -V ../DemoScene.vert -o DemoScene.vert.spv
// $ glslangValidator -V ../DemoScene.frag -o DemoScene.frag.spv
// $ glslangValidator -V ../TextureCopy.vert -o TextureCopy.vert.spv
// $ glslangValidator -V ../TextureCopy.frag -o TextureCopy.vert.spv
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragTex;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
	outColor = texture(texSampler, fragTex);
}
