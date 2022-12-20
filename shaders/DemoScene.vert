// Precompiled SPIR-V binaries are embedded in VulkanExample.cpp, this is for refenrece.
// If you wish to compile the shader by yourself, use for e.g. glslangValidator from
// https://github.com/KhronosGroup/glslang and run
// $ glslangValidator -V ../DemoScene.vert -o DemoScene.vert.spv
// $ glslangValidator -V ../DemoScene.frag -o DemoScene.frag.spv
// $ glslangValidator -V ../TextureCopy.vert -o TextureCopy.vert.spv
// $ glslangValidator -V ../TextureCopy.frag -o TextureCopy.vert.spv
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    float selection;
} ubo;

void main() {
    // Fove::Matrix44 is stored in a row-major format
    gl_Position = vec4(inPosition.xyz, 1.0) * ubo.mvp;
	const float selection = max(0.0, 0.5 - abs(ubo.selection - inPosition.w));
    fragColor = inColor + vec3(selection);
}
