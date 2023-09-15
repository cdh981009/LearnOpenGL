#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;
out vec4 position;

void main()
{
    gl_Position = vec4(aPos.x, aPos.yz, 1.0f);
    position = gl_Position;
    ourColor = aColor;
}
