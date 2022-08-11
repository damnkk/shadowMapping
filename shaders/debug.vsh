#version 330 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vTexCoord;

out vec2 texcoord;

void main()
{
    gl_Position = vec4(vPosition,1.0);
    texcoord = vTexCoord;
}