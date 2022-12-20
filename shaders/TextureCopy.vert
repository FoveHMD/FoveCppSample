// Precompiled SPIR-V binaries are embedded in VulkanExample.cpp, this is for refenrece.
// If you wish to compile the shader by yourself, use for e.g. glslangValidator from
// https://github.com/KhronosGroup/glslang and run
// $ glslangValidator -V ./DemoScene.vert -o DemoScene.vert.spv
// $ glslangValidator -V ./DemoScene.frag -o DemoScene.frag.spv
// $ glslangValidator -V ./TextureCopy.vert -o TextureCopy.vert.spv
// $ glslangValidator -V ./TextureCopy.frag -o TextureCopy.frag.spv
#version 450 core

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 tex;

layout(location = 0) out vec2 fragTex;

void main()
{
	gl_Position = vec4(pos, 0.0, 1.0);
	fragTex = tex;
}
