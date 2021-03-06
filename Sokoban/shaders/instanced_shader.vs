#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 iPos;
layout (location = 4) in vec3 iScale;
layout (location = 5) in vec2 iTexOffset;
layout (location = 6) in vec4 iColor;

out float LightIntensity;
out vec2 TexCoord;
out vec4 Color;

uniform float texScale;
uniform float lightMixFactor;
uniform vec3 lightSource;
uniform mat4 PV; //Projection * View

void main()
{
    vec3 realPos = iPos + (iScale * aPos);
	gl_Position = PV * vec4(realPos, 1);
	LightIntensity = (1 - lightMixFactor) + max(dot(aNormal, normalize(lightSource - realPos)), 0) * lightMixFactor;
    TexCoord = texScale * (iTexOffset + aTexCoord);
	Color = iColor;
}
