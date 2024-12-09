#version 330 core

in vec3 passColorAttribute;
in vec2 passUV;
out vec4 fragmentColor;

uniform sampler2D textureSampler;

void main()
{
	vec4 textureColor = texture(textureSampler, passUV);
	fragmentColor = textureColor;
};