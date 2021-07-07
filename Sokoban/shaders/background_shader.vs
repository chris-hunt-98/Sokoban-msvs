#version 330 core
layout (location = 0) in vec3 aPos;

out float z;

uniform mat4 PV;

void main()
{
  gl_Position = PV * vec4(aPos, 1);
  z = aPos.z;
}